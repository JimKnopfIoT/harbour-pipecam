/*
 * QML facade for the USeePlus USB endoscope.
 *   UppCameraWorker (QThread): libusb, handshake, reassembly, decode; no Qt Quick.
 *     Emits (QImage, original JPEG); snapshot/recording use the JPEG, no re-encode.
 *     YUYV variant: encoded to JPEG once in the worker.
 *   UppCamera (GUI thread): QML properties, latest frame, fps.
 * Worker retries open forever until stopped; disconnects are expected.
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
#include <QSize>
#include <QString>

#include "usertext.h"
#include <QVariantMap>
#include <QThread>

struct libusb_context;
struct libusb_device_handle;

class UppCameraWorker : public QThread
{
    Q_OBJECT
public:
    explicit UppCameraWorker(QObject *parent = 0);
    ~UppCameraWorker();

    /* Any thread. Takes effect within ~1 s (bulk read timeout). */
    void requestStop();

    /* gain x 100, 100 = off. Written by GUI thread, read by worker. */
    void setGainPercent(int percent);

signals:
    /* jpeg: untouched wire bytes. */
    void frameReady(const QImage &image, const QByteArray &jpeg);
    /* status: UppCamera::Status */
    void statusChanged(int status, const QString &detail);
    void buttonChanged(bool pressed);
    /* Once per successful open. */
    void deviceInfoReady(const QVariantMap &info);

protected:
    void run();

private:
    /* 0 and *out on success; else libusb error code and *err. */
    int openDevice(libusb_context *ctx, libusb_device_handle **out, UserText *err);
    int handshake(libusb_device_handle *h, UserText *err);
    /* Steps 2-6, see uppprotocol.h. */
    int handshakeMjpeg(libusb_device_handle *h, UserText *err);
    int handshakeYuyv(libusb_device_handle *h, UserText *err);
    /* Returns when stopped or on fatal USB error. */
    void streamLoop(libusb_device_handle *h);
    void streamLoopMjpeg(libusb_device_handle *h);
    void streamLoopYuyv(libusb_device_handle *h);
    void teardown(libusb_device_handle *h);

    /* In place. */
    void applyGain(QImage *image) const;

    QAtomicInt m_stop;
    QAtomicInt m_gainPercent;
    bool m_lastButton;
    /* upp::Variant */
    int m_variant;
};

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
    /* Software brightness, 1.0 = off .. maxGain. Applied in worker.
     * Affects display and snapshots (re-encoded), not video (original JPEGs). */
    Q_PROPERTY(qreal gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(qreal maxGain READ maxGain CONSTANT)
    /* From received frames: MJPEG 640x480, YUYV 320x240; 640x480 before first. */
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)
    /* false: no known LED command (header fields, CONNECT args ruled out;
     * iAP endpoint untested). UI enables the slider when true. */
    Q_PROPERTY(bool ledSupported READ ledSupported CONSTANT)

public:
    enum Status {
        Idle,
        Searching,     /* not on the bus */
        Connecting,    /* handshake */
        Streaming,
        Error          /* found but unusable, usually permissions */
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

    /* USB descriptor data, empty until first open. Contains the camera serial:
     * display only, never written to a file. */
    Q_PROPERTY(QVariantMap deviceInfo READ deviceInfo NOTIFY deviceInfoChanged)
    QVariantMap deviceInfo() const { return m_deviceInfo; }

    QImage currentImage() const { return m_image; }
    QByteArray currentJpeg() const { return m_jpeg; }

public slots:
    /* Idempotent. */
    void start();
    void stop();

signals:
    void statusChanged();
    void runningChanged();
    void fpsChanged();
    void gainChanged();
    void frameAvailable();
    void buttonPressedChanged();
    void deviceInfoChanged();
    void frameSizeChanged();
    /* Rising edge. */
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
    QSize m_frameSize;
    int m_frameCount;

    /* Counted per 1 s window; per-frame deltas too jittery. */
    qreal m_fps;
    int m_fpsFrames;
    QElapsedTimer m_fpsTimer;

    bool m_buttonPressed;
    qreal m_gain;
    QVariantMap m_deviceInfo;
};

#endif // UPPCAMERA_H
