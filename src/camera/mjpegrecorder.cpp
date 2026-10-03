/*
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

/* Max wait for moov atom after EOS; blocks the UI. */
static const int EOS_TIMEOUT_MS = 5000;

/* Caps only; real timing from buffer timestamps. */
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
        /* QML siblings; either may be destroyed first. */
        connect(m_source, SIGNAL(destroyed()), this, SLOT(onSourceDestroyed()));
    }
    emit sourceChanged();
}

void MjpegRecorder::onSourceDestroyed()
{
    /* Finalise, else no moov atom. */
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

    /* Gain exists only in the QImage, not in the camera JPEG. */
    const bool needsRender = !m_overlayText.isEmpty()
                          || !qFuzzyIsNull(m_rotation)
                          || m_source->gain() > 1.001;
    if (!needsRender) {
        pushFrame(m_source->currentJpeg());
        return;
    }

    QImage image = m_source->currentImage();
    if (image.isNull()) {
        pushFrame(m_source->currentJpeg());
        return;
    }

    /* Detach from the viewfinder's shared copy. */
    image = image.convertToFormat(QImage::Format_RGB32);
    /* Rotate before stamping: timestamp stays level. */
    image = overlay::rotateFit(image, m_rotation);
    overlay::drawTimestamp(&image, m_overlayText);

    QByteArray encoded;
    QBuffer buf(&encoded);
    buf.open(QIODevice::WriteOnly);
    /* ~source quality. */
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

void MjpegRecorder::setError(const UserText &err)
{
    m_lastError = err.ui;
    qWarning() << "pipecam: recorder:" << err.log;
    emit lastErrorChanged();
}

bool MjpegRecorder::start(const QString &path, int width, int height)
{
    if (m_pipeline) {
        setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Already recording.")));
        return false;
    }

    m_lastError.clear();
    m_frameCount = 0;
    m_bytesWritten = 0;
    m_firstFrameNs = 0;
    m_lastFrameNs = -1;

    m_pipeline = gst_pipeline_new("pipecam-recorder");
    m_appsrc = gst_element_factory_make("appsrc", "src");
    /* jpegparse required: qtmux won't negotiate bare image/jpeg caps
     * (GST_FLOW_NOT_NEGOTIATED, "Internal data stream error"). Parses only. */
    GstElement *parse = gst_element_factory_make("jpegparse", "parse");
    GstElement *mux = gst_element_factory_make("qtmux", "mux");
    GstElement *sink = gst_element_factory_make("filesink", "sink");

    if (!m_pipeline || !m_appsrc || !parse || !mux || !sink) {
        setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Video recording is unavailable: a required GStreamer "
                                                                              "element is missing (appsrc/jpegparse/qtmux/filesink).")));
        teardown();
        return false;
    }

    GstCaps *caps = gst_caps_new_simple("image/jpeg",
                                        "width",     G_TYPE_INT, width,
                                        "height",    G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, NOMINAL_FPS, 1,
                                        NULL);
    g_object_set(G_OBJECT(m_appsrc),
                 "caps", caps,
                 "format", GST_FORMAT_TIME,
                 "is-live", TRUE,
                 "do-timestamp", FALSE,       /* stamped in pushFrame() */
                 "block", FALSE,              /* push runs on GUI thread */
                 /* Queue cap; excess frames dropped. */
                 "max-bytes", (guint64)(64 * 1024 * 1024),
                 NULL);
    gst_caps_unref(caps);

    /* No faststart: second full pass over the file. */
    g_object_set(G_OBJECT(sink), "location", path.toUtf8().constData(), NULL);

    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, parse, mux, sink, NULL);
    if (!gst_element_link_many(m_appsrc, parse, mux, sink, NULL)) {
        setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Could not build the recording pipeline.")));
        teardown();
        return false;
    }

    const GstStateChangeReturn ret =
            gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Could not start recording to %1.")).arg(path));
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

    /* Copy: GStreamer owns the buffer beyond the QByteArray's lifetime. */
    GstBuffer *buf = gst_buffer_new_allocate(NULL, gsize(jpeg.size()), NULL);
    if (!buf)
        return;
    gst_buffer_fill(buf, 0, jpeg.constData(), gsize(jpeg.size()));

    GST_BUFFER_PTS(buf) = GstClockTime(ptsNs);
    GST_BUFFER_DTS(buf) = GstClockTime(ptsNs);
    /* Gap to previous frame; nominal interval for the first. */
    GST_BUFFER_DURATION(buf) =
            (m_lastFrameNs >= 0) ? GstClockTime(ptsNs - m_lastFrameNs)
                                 : GstClockTime(GST_SECOND / NOMINAL_FPS);

    const GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(m_appsrc), buf);
    if (ret != GST_FLOW_OK) {
        /* Drop frame, keep recording. */
        qWarning() << "pipecam: recorder: push_buffer returned" << ret;
        return;
    }

    m_lastFrameNs = ptsNs;
    ++m_frameCount;
    m_bytesWritten += jpeg.size();
    emit progress();

    /* Else pipeline errors surface only at stop(). */
    pollBus();
}

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
    /* Always log dbg: err->message alone hides the real cause. */
    qWarning() << "pipecam: recorder pipeline error:"
               << (err ? err->message : "unknown")
               << "| debug:" << (dbg ? dbg : "(none)");
    setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Recording failed: %1"))
             .arg(QString::fromUtf8(err ? err->message : "unknown")));
    if (err) g_error_free(err);
    g_free(dbg);
    gst_message_unref(msg);

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

    /* moov atom written only after EOS reaches the sink. */
    if (m_appsrc) {
        gst_app_src_end_of_stream(GST_APP_SRC(m_appsrc));

        GstBus *bus = gst_element_get_bus(m_pipeline);
        if (bus) {
            GstMessage *msg = gst_bus_timed_pop_filtered(
                        bus, GstClockTime(EOS_TIMEOUT_MS) * GST_MSECOND,
                        GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
            if (!msg) {
                setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Timed out finalising the video file — it may be "
                                                                                      "incomplete.")));
            } else {
                if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                    GError *err = 0;
                    gchar *dbg = 0;
                    gst_message_parse_error(msg, &err, &dbg);
                    qWarning() << "pipecam: recorder pipeline error:"
                               << (err ? err->message : "unknown")
                               << "| debug:" << (dbg ? dbg : "(none)");
                    setError(UserText("MjpegRecorder", QT_TRANSLATE_NOOP("MjpegRecorder", "Recording failed: %1"))
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

    if (!path.isEmpty() && QFileInfo(path).size() > 0)
        emit recordingFinished(path);
}

void MjpegRecorder::teardown()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        /* Also frees the children (owned by the bin). */
        gst_object_unref(GST_OBJECT(m_pipeline));
    }
    m_pipeline = 0;
    m_appsrc = 0;
    m_outputPath.clear();
}
