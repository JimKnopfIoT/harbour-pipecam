/*
 * mjpegrecorder.cpp — see mjpegrecorder.h for the rationale.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "mjpegrecorder.h"
#include "frameoverlay.h"
#include "uppcamera.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <QBuffer>
#include <QDebug>
#include <QImage>
#include <QFileInfo>

/* How long to wait for qtmux to write the moov atom after EOS. Finalising a
 * few hundred megabytes of MJPEG is fast, but the file lives on the phone's
 * eMMC and a flush can stall; five seconds is generous without hanging the UI
 * indefinitely if something has gone wrong. */
static const int EOS_TIMEOUT_MS = 5000;

/* Nominal rate advertised in the caps. The real rate is variable and is carried
 * by the per-buffer timestamps; this value only seeds the track header. */
static const int NOMINAL_FPS = 15;

void MjpegRecorder::initGStreamer(int *argc, char ***argv)
{
    GError *err = 0;
    if (!gst_init_check(argc, argv, &err)) {
        qWarning() << "pipecam: gst_init failed:" << (err ? err->message : "unknown");
        if (err)
            g_error_free(err);
    }
}

MjpegRecorder::MjpegRecorder(QObject *parent)
    : QObject(parent)
    , m_source(0)
    , m_rotation(0.0)
    , m_pipeline(0)
    , m_appsrc(0)
    , m_frameCount(0)
    , m_bytesWritten(0)
    , m_firstFrameNs(0)
    , m_lastFrameNs(-1)
{
}

MjpegRecorder::~MjpegRecorder()
{
    stop();
}

void MjpegRecorder::setSource(UppCamera *source)
{
    if (m_source == source)
        return;

    if (m_source)
        disconnect(m_source, 0, this, 0);

    m_source = source;

    if (m_source) {
        connect(m_source, SIGNAL(frameAvailable()), this, SLOT(onSourceFrame()));
        /* The camera and the recorder are siblings owned by QML; either can be
         * torn down first, so do not keep a dangling pointer. */
        connect(m_source, SIGNAL(destroyed()), this, SLOT(onSourceDestroyed()));
    }
    emit sourceChanged();
}

void MjpegRecorder::onSourceDestroyed()
{
    /* Losing the camera mid-recording would otherwise leave a file with no
     * moov atom. Close it properly first. */
    if (m_pipeline)
        stop();
    m_source = 0;
    emit sourceChanged();
}

void MjpegRecorder::setOverlayText(const QString &text)
{
    if (m_overlayText == text)
        return;
    m_overlayText = text;
    emit overlayTextChanged();
}

void MjpegRecorder::setRotation(qreal degrees)
{
    if (qFuzzyCompare(m_rotation, degrees))
        return;
    m_rotation = degrees;
    emit rotationChanged();
}

void MjpegRecorder::onSourceFrame()
{
    if (!m_pipeline || !m_source)
        return;

    /* Two things force a re-render: a burnt-in overlay, and software gain
     * (which lives in the decoded QImage, never in the camera's JPEG). Without
     * either, the camera's own bytes go straight to the muxer — no decode, no
     * encode, bit-exact. */
    const bool needsRender = !m_overlayText.isEmpty()
                          || !qFuzzyIsNull(m_rotation)
                          || m_source->gain() > 1.001;
    if (!needsRender) {
        pushFrame(m_source->currentJpeg());
        return;
    }

    QImage image = m_source->currentImage();
    if (image.isNull()) {
        /* Better a frame without the stamp than a dropped frame. */
        pushFrame(m_source->currentJpeg());
        return;
    }

    /* The worker's QImage is shared with the viewfinder; converting gives us
     * our own pixels to draw on without disturbing what is on screen. */
    image = image.convertToFormat(QImage::Format_RGB32);
    /* Rotate first, stamp second: the timestamp has to stay level and readable
     * in the finished file, not tilt with the picture. */
    image = overlay::rotateFit(image, m_rotation);
    overlay::drawTimestamp(&image, m_overlayText);

    QByteArray encoded;
    QBuffer buf(&encoded);
    buf.open(QIODevice::WriteOnly);
    /* Quality 90: the source frames are already JPEG at roughly this level, so
     * going higher would inflate the file without recovering detail that the
     * camera never captured, and going lower would visibly soften a picture
     * that is only 640x480 to begin with. */
    if (!image.save(&buf, "JPEG", 90)) {
        pushFrame(m_source->currentJpeg());
        return;
    }
    buf.close();

    pushFrame(encoded);
}

qint64 MjpegRecorder::durationMs() const
{
    if (m_lastFrameNs < 0)
        return 0;
    return m_lastFrameNs / 1000000;
}

void MjpegRecorder::setError(const QString &err)
{
    m_lastError = err;
    qWarning() << "pipecam: recorder:" << err;
    emit lastErrorChanged();
}

bool MjpegRecorder::start(const QString &path, int width, int height)
{
    if (m_pipeline) {
        setError(tr("Already recording."));
        return false;
    }

    m_lastError.clear();
    m_frameCount = 0;
    m_bytesWritten = 0;
    m_firstFrameNs = 0;
    m_lastFrameNs = -1;

    /* Build by hand rather than gst_parse_launch(): we need a typed pointer to
     * the appsrc anyway, and an explicit graph gives a precise error when one
     * element is missing from the device's plugin set. */
    m_pipeline = gst_pipeline_new("pipecam-recorder");
    m_appsrc = gst_element_factory_make("appsrc", "src");
    /* jpegparse is NOT optional, and leaving it out was a real bug.
     *
     * qtmux refuses to negotiate with the bare image/jpeg caps we can state
     * ourselves (width/height/framerate) — the pipeline dies immediately with
     * GST_FLOW_NOT_NEGOTIATED, surfaced as the wonderfully unhelpful "Internal
     * data stream error". jpegparse reads the JPEG's own headers and produces
     * the fully-specified caps the muxer insists on.
     *
     * It only PARSES: no decode, no re-encode, so the bytes still reach the
     * file exactly as the camera produced them.
     *
     * Reproduce the failure without the app:
     *   gst-launch-1.0 multifilesrc location=frame_%03d.jpg \
     *       caps="image/jpeg,framerate=15/1" ! qtmux ! filesink location=x.mp4
     * and watch it start working the moment you insert ! jpegparse !. */
    GstElement *parse = gst_element_factory_make("jpegparse", "parse");
    GstElement *mux = gst_element_factory_make("qtmux", "mux");
    GstElement *sink = gst_element_factory_make("filesink", "sink");

    if (!m_pipeline || !m_appsrc || !parse || !mux || !sink) {
        setError(tr("Video recording is unavailable: a required GStreamer "
                    "element is missing (appsrc/jpegparse/qtmux/filesink)."));
        teardown();
        return false;
    }

    /* image/jpeg straight into the muxer — qtmux accepts it and writes an
     * MJPEG track, so no decode/encode step exists in this pipeline at all. */
    GstCaps *caps = gst_caps_new_simple("image/jpeg",
                                        "width",     G_TYPE_INT, width,
                                        "height",    G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, NOMINAL_FPS, 1,
                                        NULL);
    g_object_set(G_OBJECT(m_appsrc),
                 "caps", caps,
                 "format", GST_FORMAT_TIME,   /* buffers carry real timestamps  */
                 "is-live", TRUE,             /* frames arrive as they arrive   */
                 "do-timestamp", FALSE,       /* we stamp them ourselves        */
                 "block", FALSE,              /* never stall the GUI thread     */
                 /* Cap the queue so a stalled disk costs memory, not the app.
                  * 64 MB is ~2 s of MJPEG; beyond that, dropping is correct. */
                 "max-bytes", (guint64)(64 * 1024 * 1024),
                 NULL);
    gst_caps_unref(caps);

    /* faststart would rewrite the file so it streams from the first byte, but
     * that means a full second pass over a multi-hundred-megabyte file on eMMC.
     * These recordings are played locally, so plain finalisation is the right
     * trade. */
    g_object_set(G_OBJECT(sink), "location", path.toUtf8().constData(), NULL);

    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, parse, mux, sink, NULL);
    if (!gst_element_link_many(m_appsrc, parse, mux, sink, NULL)) {
        setError(tr("Could not build the recording pipeline."));
        teardown();
        return false;
    }

    const GstStateChangeReturn ret =
            gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        setError(tr("Could not start recording to %1.").arg(path));
        teardown();
        return false;
    }

    m_outputPath = path;
    m_clock.start();
    emit recordingChanged();
    emit progress();
    return true;
}

void MjpegRecorder::pushFrame(const QByteArray &jpeg)
{
    if (!m_pipeline || !m_appsrc || jpeg.isEmpty())
        return;

    const qint64 nowNs = m_clock.nsecsElapsed();
    if (m_frameCount == 0)
        m_firstFrameNs = nowNs;
    const qint64 ptsNs = nowNs - m_firstFrameNs;

    /* Copy into a GstBuffer: the QByteArray is shared with the display path and
     * may be released the moment we return, while GStreamer owns the buffer
     * until the muxer is done with it. */
    GstBuffer *buf = gst_buffer_new_allocate(NULL, gsize(jpeg.size()), NULL);
    if (!buf)
        return;
    gst_buffer_fill(buf, 0, jpeg.constData(), gsize(jpeg.size()));

    GST_BUFFER_PTS(buf) = GstClockTime(ptsNs);
    GST_BUFFER_DTS(buf) = GstClockTime(ptsNs);
    /* Duration is only known once the NEXT frame arrives. Seed it with the
     * nominal interval; the real spacing is carried by the timestamps, which is
     * what a player actually uses. */
    GST_BUFFER_DURATION(buf) =
            (m_lastFrameNs >= 0) ? GstClockTime(ptsNs - m_lastFrameNs)
                                 : GstClockTime(GST_SECOND / NOMINAL_FPS);

    const GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(m_appsrc), buf);
    if (ret != GST_FLOW_OK) {
        /* Do not tear the recording down on a single hiccup — a dropped frame
         * is far better than a truncated file. */
        qWarning() << "pipecam: recorder: push_buffer returned" << ret;
        return;
    }

    m_lastFrameNs = ptsNs;
    ++m_frameCount;
    m_bytesWritten += jpeg.size();
    emit progress();

    /* Poll the bus as we go. Without this a pipeline that fails on its very
     * first buffer keeps "recording" happily to a file that will never contain
     * anything, and the user only finds out when they press stop. */
    pollBus();
}

/* Non-blocking check for an error on the pipeline bus. Reports it and stops the
 * recording, so the UI reflects reality immediately. */
void MjpegRecorder::pollBus()
{
    if (!m_pipeline)
        return;

    GstBus *bus = gst_element_get_bus(m_pipeline);
    if (!bus)
        return;

    GstMessage *msg = gst_bus_pop_filtered(bus, GST_MESSAGE_ERROR);
    gst_object_unref(bus);
    if (!msg)
        return;

    GError *err = 0;
    gchar *dbg = 0;
    gst_message_parse_error(msg, &err, &dbg);
    /* ALWAYS log the debug string, not just err->message. GStreamer's
     * user-facing messages are famously vague — "Internal data stream error"
     * was actually GST_FLOW_NOT_NEGOTIATED from qtmux, and only the debug
     * string said so. Throwing it away cost an entire debugging round. */
    qWarning() << "pipecam: recorder pipeline error:"
               << (err ? err->message : "unknown")
               << "| debug:" << (dbg ? dbg : "(none)");
    setError(tr("Recording failed: %1")
             .arg(QString::fromUtf8(err ? err->message : "unknown")));
    if (err) g_error_free(err);
    g_free(dbg);
    gst_message_unref(msg);

    /* The pipeline is dead; do not pretend otherwise. */
    teardown();
    emit recordingChanged();
    emit progress();
}

void MjpegRecorder::stop()
{
    if (!m_pipeline) {
        return;
    }

    const QString path = m_outputPath;

    /* Signal end-of-stream and wait for it to reach the sink. Only then has
     * qtmux written the moov atom and the .mp4 become playable. */
    if (m_appsrc) {
        gst_app_src_end_of_stream(GST_APP_SRC(m_appsrc));

        GstBus *bus = gst_element_get_bus(m_pipeline);
        if (bus) {
            GstMessage *msg = gst_bus_timed_pop_filtered(
                        bus, GstClockTime(EOS_TIMEOUT_MS) * GST_MSECOND,
                        GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
            if (!msg) {
                setError(tr("Timed out finalising the video file — it may be "
                            "incomplete."));
            } else {
                if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                    GError *err = 0;
                    gchar *dbg = 0;
                    gst_message_parse_error(msg, &err, &dbg);
                    qWarning() << "pipecam: recorder pipeline error:"
                               << (err ? err->message : "unknown")
                               << "| debug:" << (dbg ? dbg : "(none)");
                    setError(tr("Recording failed: %1")
                             .arg(QString::fromUtf8(err ? err->message : "unknown")));
                    if (err) g_error_free(err);
                    g_free(dbg);
                }
                gst_message_unref(msg);
            }
            gst_object_unref(bus);
        }
    }

    teardown();
    emit recordingChanged();
    emit progress();

    /* Only announce a file that actually has content. */
    if (!path.isEmpty() && QFileInfo(path).size() > 0)
        emit recordingFinished(path);
}

void MjpegRecorder::teardown()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        /* Unreffing the pipeline drops its children (appsrc, qtmux, filesink)
         * with it — they were adopted by gst_bin_add_many(). */
        gst_object_unref(GST_OBJECT(m_pipeline));
    }
    m_pipeline = 0;
    m_appsrc = 0;
    m_outputPath.clear();
}
