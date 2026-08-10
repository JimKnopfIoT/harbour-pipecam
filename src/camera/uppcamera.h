/*
 * uppcamera.h — QML-facing facade for the USeePlus USB endoscope.
 *
 * DESIGN
 * ------
 * Two objects, two threads:
 *
 *   UppCameraWorker (QThread)   owns the libusb handle and does nothing but
 *                               open -> handshake -> read -> reassemble ->
 *                               decode. It never touches Qt Quick. It emits a
 *                               finished frame as (QImage, QByteArray) — the
 *                               decoded image for display and the ORIGINAL
 *                               JPEG bytes for snapshots and recording, so
 *                               neither path ever re-encodes.
 *
 *   UppCamera (QObject)         lives in the GUI thread, exposed to QML. Holds
 *                               the most recent frame, computes fps, and turns
 *                               worker signals into QML properties.
 *
 * Why the worker decodes: a 640x480 JPEG costs a few milliseconds to decode.
 * At 15 fps that is a noticeable slice of the GUI thread's frame budget, and
 * it is trivially parallel work with no Qt Quick dependency. Decoding in the
 * worker keeps the UI thread free for the UI.
 *
 * Why we keep the raw JPEG too: the camera hands us MJPEG. A snapshot is then
 * literally "write these bytes to a file" and a recording is "mux these bytes"
 * (see MjpegRecorder). Re-encoding a decoded QImage would cost CPU and quality
 * for nothing.
 *
 * RESILIENCE
 * ----------
 * The cable is ~10 m and gets moved around a pipe; disconnects are expected,
 * not exceptional. The worker therefore runs a supervision loop: if the device
 * is absent or a transfer fails, it reports the state, waits, and retries
 * forever until asked to stop. QML just watches `status`.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef UPPCAMERA_H
#define UPPCAMERA_H

#include <QAtomicInt>
#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QThread>

struct libusb_context;
struct libusb_device_handle;

/* ------------------------------------------------------------------------- */
/* Worker thread: raw USB, no Qt Quick.                                      */
/* ------------------------------------------------------------------------- */
class UppCameraWorker : public QThread
{
    Q_OBJECT
public:
    explicit UppCameraWorker(QObject *parent = 0);
    ~UppCameraWorker();

    /* Ask the loop to unwind. Safe from any thread. The bulk reads use a 1 s
     * timeout, so the thread notices within ~1 s without needing to be killed. */
    void requestStop();

    /* Brightness multiplier, as gain x 100 (100 = off). Stored as an atomic int
     * because the GUI thread writes it while the worker reads it every frame,
     * and a full mutex for one integer that can safely be a frame stale would
     * be ceremony. */
    void setGainPercent(int percent);

signals:
    /* image: decoded frame for display. jpeg: the untouched bytes off the wire. */
    void frameReady(const QImage &image, const QByteArray &jpeg);
    /* status is a UppCamera::Status value; int keeps this class independent. */
    void statusChanged(int status, const QString &detail);
    void buttonChanged(bool pressed);
    /* Emitted once per successful open, with the descriptor strings. */
    void deviceInfoReady(const QVariantMap &info);

protected:
    void run();

private:
    /* Find and open the camera. Returns 0 and sets *out on success, otherwise a
     * libusb error code with *err describing what a user can do about it. */
    int openDevice(libusb_context *ctx, libusb_device_handle **out, QString *err);
    /* Steps 2-6 of the handshake documented in uppprotocol.h. */
    int handshake(libusb_device_handle *h, QString *err);
    /* The read/reassemble loop. Returns when stopped or on a fatal USB error. */
    void streamLoop(libusb_device_handle *h);

    /* Apply the current gain to a freshly decoded frame, in place. */
    void applyGain(QImage *image) const;

    QAtomicInt m_stop;
    QAtomicInt m_gainPercent;
    bool m_lastButton;
};

/* ------------------------------------------------------------------------- */
/* GUI-thread facade, registered as a QML type.                              */
/* ------------------------------------------------------------------------- */
class UppCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString statusDetail READ statusDetail NOTIFY statusChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY statusChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(qreal fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY frameAvailable)
    Q_PROPERTY(bool buttonPressed READ buttonPressed NOTIFY buttonPressedChanged)
    /* Software brightness, 1.0 = untouched, up to maxGain.
     *
     * This exists because the LED ring cannot be dimmed from here (proven —
     * see the “Protocol” section of README.md) and a pipe is dark. It is applied in the worker
     * thread, right after the JPEG is decoded, so the GUI thread never does
     * pixel work.
     *
     * It affects what you see AND what a snapshot contains (a snapshot with
     * gain has to be re-encoded, exactly like the burnt-in timestamp). It does
     * NOT affect video: recording muxes the camera's original JPEGs, and
     * re-encoding every frame to brighten it would cost far more than it is
     * worth. */
    Q_PROPERTY(qreal gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(qreal maxGain READ maxGain CONSTANT)
    Q_PROPERTY(int frameWidth READ frameWidth CONSTANT)
    Q_PROPERTY(int frameHeight READ frameHeight CONSTANT)
    /* Whether the LED ring on the camera head can be driven from here.
     *
     * Currently false, and that is a measured result rather than a stub: 45 s
     * of passive capture showed no header field reacting to the cable's dimmer
     * wheel, no LED command exists in any reverse-engineered reference, and
     * sweeping the CONNECT command's two unexplained argument bytes changed
     * nothing (see the “Protocol” section of README.md). The untried lever is the iAP control
     * endpoint. The UI reads this flag, so proving a command and returning true
     * here is all that is needed to light the slider up. */
    Q_PROPERTY(bool ledSupported READ ledSupported CONSTANT)

public:
    enum Status {
        Idle,          /* stopped by the user                                  */
        Searching,     /* no camera on the bus — cable unplugged?              */
        Connecting,    /* found it, running the handshake                      */
        Streaming,     /* frames arriving                                      */
        Error          /* found it but could not use it (usually permissions)  */
    };
    Q_ENUMS(Status)

    explicit UppCamera(QObject *parent = 0);
    ~UppCamera();

    Status status() const { return m_status; }
    QString statusText() const;
    QString statusDetail() const { return m_statusDetail; }
    bool streaming() const { return m_status == Streaming; }
    bool running() const { return m_worker != 0; }
    qreal fps() const { return m_fps; }
    int frameCount() const { return m_frameCount; }
    bool buttonPressed() const { return m_buttonPressed; }
    qreal gain() const { return m_gain; }
    void setGain(qreal gain);
    qreal maxGain() const;
    int frameWidth() const;
    int frameHeight() const;
    bool ledSupported() const { return false; }

    /* Everything the device itself tells us, read from the USB descriptors when
     * the camera is opened: manufacturer, product, serial, VID:PID, bus
     * address, USB speed. Exposed as a QVariantMap so the specs page can list
     * it without a bespoke model, and empty until a camera has been opened
     * once. Anonymity note: the serial is the CAMERA's, not the phone's, and it
     * is only ever shown on screen — nothing writes it to a file. */
    Q_PROPERTY(QVariantMap deviceInfo READ deviceInfo NOTIFY deviceInfoChanged)
    QVariantMap deviceInfo() const { return m_deviceInfo; }

    /* Latest decoded frame, for the render item. Cheap: QImage is COW. */
    QImage currentImage() const { return m_image; }
    /* Latest untouched JPEG, for snapshots and the recorder. */
    QByteArray currentJpeg() const { return m_jpeg; }

public slots:
    /* Start/stop the supervision thread. Idempotent. */
    void start();
    void stop();

signals:
    void statusChanged();
    void runningChanged();
    void fpsChanged();
    void gainChanged();
    /* A new frame is in currentImage()/currentJpeg(). */
    void frameAvailable();
    void buttonPressedChanged();
    void deviceInfoChanged();
    /* Rising edge of the inline push-button — QML binds a snapshot to this. */
    void buttonClicked();

private slots:
    void onFrameReady(const QImage &image, const QByteArray &jpeg);
    void onStatusChanged(int status, const QString &detail);
    void onButtonChanged(bool pressed);
    void onDeviceInfoReady(const QVariantMap &info);

private:
    void setStatus(Status s, const QString &detail);

    UppCameraWorker *m_worker;
    Status m_status;
    QString m_statusDetail;
    QImage m_image;
    QByteArray m_jpeg;
    int m_frameCount;

    /* fps is measured over a sliding one-second window rather than from frame
     * deltas — the per-frame interval is far too jittery to display. */
    qreal m_fps;
    int m_fpsFrames;
    QElapsedTimer m_fpsTimer;

    bool m_buttonPressed;
    qreal m_gain;
    QVariantMap m_deviceInfo;
};

#endif // UPPCAMERA_H
