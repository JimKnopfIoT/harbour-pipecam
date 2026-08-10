/*
 * mjpegrecorder.h — records the camera's MJPEG stream to an .mp4 file.
 *
 * WHY THIS IS NOT A VIDEO ENCODER
 * -------------------------------
 * The camera already hands us a sequence of complete JPEG images. An MP4 file
 * can carry exactly that as a Motion-JPEG video track. So recording is pure
 * multiplexing: we hand the untouched JPEG bytes to GStreamer's `qtmux` and it
 * writes the container around them. Nothing is decoded, nothing is re-encoded.
 *
 * The one exception is deliberate: when a timestamp overlay or software gain is
 * switched on, each frame has to be drawn on and re-encoded before it is muxed
 * (see onSourceFrame). The pipeline is unchanged — it still receives JPEGs —
 * only their provenance differs. Everything below still applies.
 *
 * That matters on this device. The Xperia 10 III's Sailfish image ships no
 * usable H.264 encoder element — there is no `x264enc` and no `v4l2h264enc`
 * (verified 2026-08-10); the Venus hardware encoder at /dev/video33 is only
 * reachable through the Android HAL. Transcoding to H.264 would therefore have
 * to happen in software we do not have. Muxing MJPEG avoids the problem
 * entirely, costs almost no CPU, and is bit-exact with what the sensor sent.
 *
 * The trade-off is file size: MJPEG has no inter-frame compression, so expect
 * roughly 20-30 MB per minute at 640x480/15 fps. For pipe inspection footage
 * that is an acceptable price for zero generational loss and a recording path
 * that cannot drop frames because the encoder fell behind.
 *
 * PIPELINE
 *   appsrc (image/jpeg) -> qtmux -> filesink
 *
 * TIMING
 * ------
 * The camera's frame rate is nominally ~15.5 fps but genuinely variable — it
 * drops when the scene is detailed and the JPEGs get bigger. We therefore do
 * NOT let appsrc stamp buffers itself (`do-timestamp=false`). Each buffer gets
 * a presentation timestamp taken from a monotonic clock started at the first
 * frame, and the previous buffer's duration is corrected to the real gap. The
 * result plays back at true wall-clock speed instead of drifting.
 *
 * FINALISATION
 * ------------
 * An MP4 is only playable once its `moov` atom is written, which qtmux does on
 * end-of-stream. stop() therefore pushes EOS and waits for the bus to confirm
 * it before tearing the pipeline down. Skipping that wait produces a
 * zero-length, unplayable file — the single most common way to get this wrong.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef MJPEGRECORDER_H
#define MJPEGRECORDER_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QString>

typedef struct _GstElement GstElement;

class UppCamera;

class MjpegRecorder : public QObject
{
    Q_OBJECT
    /* The camera to record. Setting it wires frame delivery up in C++ so the
     * JPEG bytes never pass through QML — QML would convert a QByteArray to a
     * JavaScript string and corrupt every byte above 0x7F. */
    Q_PROPERTY(UppCamera *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY recordingChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY progress)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY progress)
    Q_PROPERTY(qint64 bytesWritten READ bytesWritten NOTIFY progress)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    /* Text to burn into every recorded frame; empty means none.
     *
     * Setting it changes how recording works, and the trade-off is worth
     * stating. Normally the recorder muxes the camera's own JPEGs untouched —
     * no decode, no encode, bit-exact. With an overlay (or with software gain
     * on) every frame has to be drawn on and re-encoded, which costs a few
     * milliseconds per frame and one generation of JPEG quality.
     *
     * That is the right trade for an inspection recording: a video with no date
     * on it is much harder to use as documentation months later than a video
     * that has been through one extra JPEG round. */
    Q_PROPERTY(QString overlayText READ overlayText WRITE setOverlayText
               NOTIFY overlayTextChanged)
    /* Roll applied to every recorded frame, in degrees; 0 records the camera's
     * own orientation.
     *
     * The viewfinder's roll is a display transform — it does not touch the
     * frames, so without this a recording of a sideways pipe stays sideways
     * however the picture was straightened on screen. Like the overlay, a
     * non-zero angle forces the render-and-re-encode path. */
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)

public:
    explicit MjpegRecorder(QObject *parent = 0);
    ~MjpegRecorder();

    /* Must be called once before any recorder is constructed. Wraps gst_init(). */
    static void initGStreamer(int *argc, char ***argv);

    UppCamera *source() const { return m_source; }
    void setSource(UppCamera *source);

    bool recording() const { return m_pipeline != 0; }
    QString outputPath() const { return m_outputPath; }
    int frameCount() const { return m_frameCount; }
    qint64 durationMs() const;
    qint64 bytesWritten() const { return m_bytesWritten; }
    QString lastError() const { return m_lastError; }
    QString overlayText() const { return m_overlayText; }
    void setOverlayText(const QString &text);
    qreal rotation() const { return m_rotation; }
    void setRotation(qreal degrees);

    /* NOTE: start() and stop() MUST stay Q_INVOKABLE — QML cannot call a plain
     * public C++ method. Without it `recorder.start(...)` fails silently at
     * runtime (a TypeError in the journal, nothing on screen), which looks
     * exactly like "the record button does nothing". */

    /* Start writing to `path`. `width`/`height` describe the JPEGs that will be
     * pushed; they go into the caps so the muxer can write a correct track
     * header. Returns false and sets lastError() on failure. */
    Q_INVOKABLE bool start(const QString &path, int width, int height);

    /* Push EOS, wait for the muxer to finalise, tear down. Safe to call when
     * not recording. */
    Q_INVOKABLE void stop();

    /* Hand one complete JPEG frame to the muxer. Ignored when not recording.
     * Not invokable on purpose: frames are pulled from `source` inside C++ so
     * the bytes never cross the QML boundary. */
    void pushFrame(const QByteArray &jpeg);

signals:
    void sourceChanged();
    void overlayTextChanged();
    void rotationChanged();
    void recordingChanged();
    void progress();
    void lastErrorChanged();
    /* Emitted once a file is closed and complete, so the gallery can pick it up. */
    void recordingFinished(const QString &path);

private slots:
    /* Connected to the source camera's frameAvailable(). Pulls the frame and
     * pushes it, but only while recording. */
    void onSourceFrame();
    void onSourceDestroyed();

private:
    void teardown();
    void setError(const QString &err);
    /* Non-blocking bus check, run while frames are flowing. */
    void pollBus();

    UppCamera *m_source;
    QString m_overlayText;
    qreal m_rotation;
    GstElement *m_pipeline;
    GstElement *m_appsrc;
    QString m_outputPath;
    QString m_lastError;
    int m_frameCount;
    qint64 m_bytesWritten;
    /* Nanoseconds, relative to the first pushed frame. */
    qint64 m_firstFrameNs;
    qint64 m_lastFrameNs;
    QElapsedTimer m_clock;
};

#endif // MJPEGRECORDER_H
