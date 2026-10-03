/*
 * Records the camera stream to .mp4: appsrc (image/jpeg) -> jpegparse -> qtmux -> filesink.
 * Muxes original JPEGs, no re-encode, except with overlay/rotation/gain.
 * No H.264 encoder element on the target image (no x264enc/v4l2h264enc).
 * ~20-30 MB/min at 640x480/15 fps.
 * Timestamps from a monotonic clock (do-timestamp=false); frame rate is variable.
 * stop() must wait for EOS on the bus: moov atom written only then, else unplayable.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef MJPEGRECORDER_H
#define MJPEGRECORDER_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QString>

#include "usertext.h"

typedef struct _GstElement GstElement;

class UppCamera;

class MjpegRecorder : public QObject
{
    Q_OBJECT
    /* Frames wired in C++: QML converts QByteArray to string, corrupts bytes > 0x7F. */
    Q_PROPERTY(UppCamera *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY recordingChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY progress)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY progress)
    Q_PROPERTY(qint64 bytesWritten READ bytesWritten NOTIFY progress)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    /* Burnt into every frame; empty = none. Non-empty forces re-encode. */
    Q_PROPERTY(QString overlayText READ overlayText WRITE setOverlayText
               NOTIFY overlayTextChanged)
    /* Degrees, applied to recorded frames. Non-zero forces re-encode. */
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)

public:
    explicit MjpegRecorder(QObject *parent = 0);
    ~MjpegRecorder();

    /* Once, before any recorder is constructed. */
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

    /* start()/stop() must stay Q_INVOKABLE (called from QML). */

    /* width/height: pixel size of pushed JPEGs (caps). false + lastError() on failure. */
    Q_INVOKABLE bool start(const QString &path, int width, int height);

    /* Blocks until EOS (max 5 s). No-op when not recording. */
    Q_INVOKABLE void stop();

    /* Not Q_INVOKABLE on purpose: bytes must not cross into QML. */
    void pushFrame(const QByteArray &jpeg);

signals:
    void sourceChanged();
    void overlayTextChanged();
    void rotationChanged();
    void recordingChanged();
    void progress();
    void lastErrorChanged();
    /* File closed and non-empty. */
    void recordingFinished(const QString &path);

private slots:
    void onSourceFrame();
    void onSourceDestroyed();

private:
    void teardown();
    void setError(const UserText &err);
    /* Non-blocking. */
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
    /* ns, relative to first pushed frame. */
    qint64 m_firstFrameNs;
    qint64 m_lastFrameNs;
    QElapsedTimer m_clock;
};

#endif // MJPEGRECORDER_H
