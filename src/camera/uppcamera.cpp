/*
 * uppcamera.cpp — see uppcamera.h for the design, uppprotocol.h for the wire
 * format. The reassembly here is the same algorithm proven by
 * the reference probe described in README.md against the real device.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "uppcamera.h"
#include "uppprotocol.h"
#include "diaglog.h"

#include <libusb-1.0/libusb.h>

#include <QDebug>
#include <QStringList>

/* How long to wait before re-scanning the bus after a failure. Long enough not
 * to spin on a missing camera, short enough that re-plugging feels instant. */
static const int RETRY_DELAY_MS = 1000;
/* Bulk read timeout. Also the granularity at which the thread notices a stop
 * request, so it must stay comfortably under a second of UI lag. */
static const int READ_TIMEOUT_MS = 1000;

static QString usbErr(int rc)
{
    return QString::fromLatin1(libusb_error_name(rc));
}

/* libusb's own debug output, verbose mode only. Straight into the diagnostic
 * log rather than through qDebug: at debug level libusb is chatty, and none of
 * it belongs in the system journal. */
static void LIBUSB_CALL libusbLog(libusb_context *, enum libusb_log_level, const char *str)
{
    QString line = QString::fromLocal8Bit(str);
    while (line.endsWith(QLatin1Char('\n')))
        line.chop(1);
    DiagLog::instance()->append(QStringLiteral("libusb: ") + line);
}

/* Debug while finding and opening the camera — that is where a failure needs
 * explaining — and back to warnings once streaming, where debug level would
 * log every one of ~1000 bulk transfers a second. */
static void setLibusbVerbose(libusb_context *ctx, bool on)
{
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL,
                      on ? LIBUSB_LOG_LEVEL_DEBUG : LIBUSB_LOG_LEVEL_WARNING);
}

/* =========================================================================
 * UppCameraWorker
 * ========================================================================= */

/* Highest software brightness offered. Beyond about 3x a 640x480 JPEG from a
 * dark pipe is mostly amplified compression noise, and pushing further makes
 * the picture worse rather than more readable. */
static const qreal MAX_GAIN = 3.0;

UppCameraWorker::UppCameraWorker(QObject *parent)
    : QThread(parent)
    , m_stop(0)
    , m_gainPercent(100)
    , m_lastButton(false)
{
}

void UppCameraWorker::setGainPercent(int percent)
{
    m_gainPercent.storeRelease(percent);
}

/* Brightness, applied straight after the JPEG decode and before the frame is
 * handed to the GUI thread.
 *
 * A lookup table rather than a multiply per channel: there are only 256
 * possible input values, and building the table once per frame turns three
 * multiplies and three clamps per pixel into three array reads. At 640x480 and
 * 15 fps that is the difference between a measurable cost and none.
 *
 * The curve is a plain linear gain with clipping, not a gamma or a tone map:
 * this is an inspection tool, and a photographic curve that lifts shadows while
 * rolling off highlights would misrepresent how bright things actually are. */
void UppCameraWorker::applyGain(QImage *image) const
{
    const int percent = m_gainPercent.loadAcquire();
    if (percent <= 100 || image->isNull())
        return;

    unsigned char lut[256];
    for (int i = 0; i < 256; ++i) {
        const int v = i * percent / 100;
        lut[i] = (unsigned char)(v > 255 ? 255 : v);
    }

    /* fromData() gives us RGB32 for a colour JPEG; convert if it ever does not,
     * so the pointer arithmetic below is always valid. */
    if (image->format() != QImage::Format_RGB32 &&
        image->format() != QImage::Format_ARGB32)
        *image = image->convertToFormat(QImage::Format_RGB32);

    const int h = image->height();
    for (int y = 0; y < h; ++y) {
        /* scanLine() on a non-const QImage detaches once, then hands back the
         * real row — no per-row copying. */
        uchar *p = image->scanLine(y);
        const int bytes = image->bytesPerLine();
        /* BGRA in memory on little-endian; the alpha byte is left alone. */
        for (int x = 0; x < bytes; x += 4) {
            p[x]     = lut[p[x]];
            p[x + 1] = lut[p[x + 1]];
            p[x + 2] = lut[p[x + 2]];
        }
    }
}

UppCameraWorker::~UppCameraWorker()
{
    requestStop();
    wait(3000);
}

void UppCameraWorker::requestStop()
{
    m_stop.storeRelease(1);
}

int UppCameraWorker::openDevice(libusb_context *ctx, libusb_device_handle **out,
                                QString *err)
{
    for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i) {
        libusb_device_handle *h = libusb_open_device_with_vid_pid(
                    ctx, upp::KNOWN_DEVICES[i].vid, upp::KNOWN_DEVICES[i].pid);
        if (h) {
            libusb_device *dev = libusb_get_device(h);
            PIPECAM_TRACE(QStringLiteral("opened %1:%2 at bus %3 address %4")
                          .arg(upp::KNOWN_DEVICES[i].vid, 4, 16, QChar('0'))
                          .arg(upp::KNOWN_DEVICES[i].pid, 4, 16, QChar('0'))
                          .arg(libusb_get_bus_number(dev))
                          .arg(libusb_get_device_address(dev)));
            *out = h;
            return 0;
        }
    }

    /* libusb_open_device_with_vid_pid() collapses "absent" and "present but
     * not permitted" into a null handle, so ask the device list which it was.
     * The difference matters a lot to the user: one means "check your cable",
     * the other means "the udev rule is missing". */
    libusb_device **list = 0;
    ssize_t n = libusb_get_device_list(ctx, &list);
    bool seen = false;
    for (ssize_t d = 0; d < n && !seen; ++d) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(list[d], &desc) != 0)
            continue;
        for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i) {
            if (desc.idVendor == upp::KNOWN_DEVICES[i].vid &&
                desc.idProduct == upp::KNOWN_DEVICES[i].pid) {
                seen = true;
                break;
            }
        }
    }
    if (n >= 0)
        libusb_free_device_list(list, 1);

    if (seen) {
        *err = QObject::tr("Camera found but not accessible. The USB permission "
                           "rule is missing — reinstall the app and replug the camera.");
        return LIBUSB_ERROR_ACCESS;
    }
    *err = QObject::tr("No camera detected. Check the USB-C connection.");
    return LIBUSB_ERROR_NO_DEVICE;
}

/* Read a USB string descriptor into a QString, or return an empty string. */
static QString usbString(libusb_device_handle *h, uint8_t index)
{
    if (index == 0)
        return QString();
    unsigned char buf[256];
    const int n = libusb_get_string_descriptor_ascii(h, index, buf, sizeof(buf));
    if (n <= 0)
        return QString();
    return QString::fromLatin1(reinterpret_cast<char *>(buf), n);
}

/* Everything the device is willing to say about itself. Gathered once per open
 * and handed to the GUI so the specs page can show real values rather than the
 * constants we happen to have compiled in. */
static QVariantMap collectDeviceInfo(libusb_device_handle *h)
{
    QVariantMap info;
    libusb_device *dev = libusb_get_device(h);
    if (!dev)
        return info;

    libusb_device_descriptor d;
    if (libusb_get_device_descriptor(dev, &d) != 0)
        return info;

    info["manufacturer"] = usbString(h, d.iManufacturer);
    info["product"]      = usbString(h, d.iProduct);
    info["serial"]       = usbString(h, d.iSerialNumber);
    info["vendorId"]     = QString("%1").arg(d.idVendor, 4, 16, QChar('0'));
    info["productId"]    = QString("%1").arg(d.idProduct, 4, 16, QChar('0'));
    info["usbVersion"]   = QString("%1.%2").arg(d.bcdUSB >> 8, 0, 16)
                                           .arg((d.bcdUSB >> 4) & 0xF, 0, 16);
    info["deviceVersion"] = QString("%1.%2").arg(d.bcdDevice >> 8, 0, 16)
                                            .arg((d.bcdDevice >> 4) & 0xF, 0, 16);
    info["busNumber"]    = int(libusb_get_bus_number(dev));
    info["deviceAddress"] = int(libusb_get_device_address(dev));

    switch (libusb_get_device_speed(dev)) {
    case LIBUSB_SPEED_LOW:   info["speed"] = QString("1.5 Mbit/s (low)");   break;
    case LIBUSB_SPEED_FULL:  info["speed"] = QString("12 Mbit/s (full)");   break;
    case LIBUSB_SPEED_HIGH:  info["speed"] = QString("480 Mbit/s (high)");  break;
    case LIBUSB_SPEED_SUPER: info["speed"] = QString("5 Gbit/s (super)");   break;
    default:                 info["speed"] = QString("unknown");            break;
    }

    info["deviceClass"] = QString("%1/%2/%3")
            .arg(d.bDeviceClass, 2, 16, QChar('0'))
            .arg(d.bDeviceSubClass, 2, 16, QChar('0'))
            .arg(d.bDeviceProtocol, 2, 16, QChar('0'));

    /* The interface classes are the whole reason this camera needs a custom
     * driver, so show them rather than making the user take our word for it. */
    libusb_config_descriptor *cfg = 0;
    if (libusb_get_active_config_descriptor(dev, &cfg) == 0 && cfg) {
        QStringList ifaces;
        for (int i = 0; i < cfg->bNumInterfaces; ++i) {
            const libusb_interface_descriptor *alt = cfg->interface[i].altsetting;
            if (!alt)
                continue;
            ifaces << QString("%1: class %2/%3/%4")
                      .arg(alt->bInterfaceNumber)
                      .arg(alt->bInterfaceClass, 2, 16, QChar('0'))
                      .arg(alt->bInterfaceSubClass, 2, 16, QChar('0'))
                      .arg(alt->bInterfaceProtocol, 2, 16, QChar('0'));
        }
        info["interfaces"] = ifaces.join(QLatin1String("\n"));
        info["maxPower"] = QString("%1 mA").arg(cfg->MaxPower * 2);
        libusb_free_config_descriptor(cfg);
    }

    return info;
}

int UppCameraWorker::handshake(libusb_device_handle *h, QString *err)
{
    int rc;

    /* No kernel driver claims this device (it is vendor-class), but detach
     * defensively so we also work on a system where someone loaded one of the
     * out-of-tree supercamera modules. */
    libusb_set_auto_detach_kernel_driver(h, 1);

    /* BUSY here just means the configuration is already active — not an error. */
    rc = libusb_set_configuration(h, 1);
    if (rc < 0 && rc != LIBUSB_ERROR_BUSY)
        qWarning() << "pipecam: set_configuration:" << libusb_error_name(rc);
    PIPECAM_TRACE(QStringLiteral("set_configuration(1): %1").arg(rc == 0 ? QStringLiteral("ok") : usbErr(rc)));

    /* Both interfaces, or the stream never starts.
     *
     * Each failure gets its own message with the libusb code attached: in
     * 0.1.1 every one of them read "camera is busy", which turned the first
     * field report into guesswork. The code is also what a user can quote. */
    for (int iface = upp::IFACE_IAP; iface <= upp::IFACE_STREAM; ++iface) {
        if (DiagLog::isVerbose()) {
            const int k = libusb_kernel_driver_active(h, iface);
            PIPECAM_TRACE(QStringLiteral("interface %1: kernel driver active = %2")
                          .arg(iface).arg(k == 0 ? QStringLiteral("no") : k == 1 ? QStringLiteral("yes") : usbErr(k)));
        }
        rc = libusb_claim_interface(h, iface);
        PIPECAM_TRACE(QStringLiteral("claim interface %1: %2").arg(iface).arg(rc == 0 ? QStringLiteral("ok") : usbErr(rc)));
        if (rc < 0) {
            qWarning() << "pipecam: claim interface" << iface << "failed:" << libusb_error_name(rc);
            switch (rc) {
            case LIBUSB_ERROR_ACCESS:
                *err = QObject::tr("Not allowed to open the camera (USB permissions).");
                break;
            case LIBUSB_ERROR_BUSY:
                *err = QObject::tr("Camera is in use by another program or driver "
                                   "(interface %1). Close other camera apps, or "
                                   "unplug and replug it.").arg(iface);
                break;
            case LIBUSB_ERROR_NOT_FOUND:
                *err = QObject::tr("This camera has no interface %1 — an unknown "
                                   "variant. Please send a diagnostic report "
                                   "(Settings → About).").arg(iface);
                break;
            default:
                *err = QObject::tr("Could not take over the camera's USB "
                                   "interface %1.").arg(iface);
                break;
            }
            *err += QStringLiteral(" [%1]").arg(usbErr(rc));
            /* Give back what was claimed, so the retry starts clean. */
            for (int j = upp::IFACE_IAP; j < iface; ++j)
                libusb_release_interface(h, j);
            return rc;
        }
    }

    /* Drain the stale iAP heartbeat; a leftover one desynchronises the rest. */
    {
        unsigned char scratch[512];
        int transferred = 0;
        for (int i = 0; i < 30; ++i) {
            if (libusb_bulk_transfer(h, upp::EP_IAP_IN, scratch, sizeof(scratch),
                                     &transferred, 100) != 0)
                break;
        }
    }

    rc = libusb_set_interface_alt_setting(h, upp::IFACE_STREAM, upp::STREAM_ALTSETTING);
    PIPECAM_TRACE(QStringLiteral("alt setting %1 on interface %2: %3").arg(upp::STREAM_ALTSETTING)
                  .arg(upp::IFACE_STREAM).arg(rc == 0 ? QStringLiteral("ok") : usbErr(rc)));
    if (rc < 0) {
        *err = QObject::tr("Could not start the camera's video interface.")
             + QStringLiteral(" [%1]").arg(usbErr(rc));
        return rc;
    }
    libusb_clear_halt(h, upp::EP_STREAM_OUT);

    int transferred = 0;
    rc = libusb_bulk_transfer(h, upp::EP_IAP_OUT,
                              const_cast<unsigned char *>(upp::MAGIC_INIT),
                              upp::MAGIC_INIT_LEN, &transferred, 1000);
    PIPECAM_TRACE(QStringLiteral("init command: %1, %2 bytes").arg(rc == 0 ? QStringLiteral("ok") : usbErr(rc)).arg(transferred));
    if (rc < 0) {
        *err = QObject::tr("Camera did not accept the initialisation command.")
             + QStringLiteral(" [%1]").arg(usbErr(rc));
        return rc;
    }
    rc = libusb_bulk_transfer(h, upp::EP_STREAM_OUT,
                              const_cast<unsigned char *>(upp::CONNECT_CMD),
                              upp::CONNECT_CMD_LEN, &transferred, 1000);
    PIPECAM_TRACE(QStringLiteral("connect command: %1, %2 bytes").arg(rc == 0 ? QStringLiteral("ok") : usbErr(rc)).arg(transferred));
    if (rc < 0) {
        *err = QObject::tr("Camera did not accept the connect command.")
             + QStringLiteral(" [%1]").arg(usbErr(rc));
        return rc;
    }
    return 0;
}

void UppCameraWorker::streamLoop(libusb_device_handle *h)
{
    QByteArray packet(upp::PKT_SIZE, Qt::Uninitialized);
    QByteArray acc;                 /* frame accumulator */
    acc.reserve(64 * 1024);
    int curFid = -1;                /* -1 = no frame started yet */
    int emitted = 0;                /* counts frames closed, incl. warm-up */

    while (!m_stop.loadAcquire()) {
        int n = 0;
        int rc = libusb_bulk_transfer(h, upp::EP_STREAM_IN,
                                      reinterpret_cast<unsigned char *>(packet.data()),
                                      upp::PKT_SIZE, &n, READ_TIMEOUT_MS);
        if (rc < 0) {
            if (rc == LIBUSB_ERROR_TIMEOUT)
                continue;           /* idle camera, not an error */
            /* Anything else (NO_DEVICE, IO, PIPE) means the cable moved or the
             * device reset. Leave and let the supervision loop reconnect. */
            qWarning() << "pipecam: stream ended:" << libusb_error_name(rc)
                       << "after" << emitted << "frames";
            return;
        }
        if (n < upp::PAYLOAD_OFFSET)
            continue;

        const unsigned char *p = reinterpret_cast<const unsigned char *>(packet.constData());
        if (p[0] != upp::MAGIC_0 || p[1] != upp::MAGIC_1 || !upp::cidIsValid(p[2]))
            continue;               /* not a video packet */

        const int length = int(p[3]) | (int(p[4]) << 8);
        const unsigned char fid   = p[5];
        const unsigned char flags = p[7];

        /* `length` is measured from offset 5; clamp to what actually arrived. */
        int end = upp::USB_HDR_LEN + length;
        if (end > n)
            end = n;
        const int chunkLen = (end > upp::PAYLOAD_OFFSET) ? end - upp::PAYLOAD_OFFSET : 0;

        const bool button = (flags & upp::FLAG_BUTTON) != 0;
        if (button != m_lastButton) {
            m_lastButton = button;
            emit buttonChanged(button);
        }

        /* A change of frame id closes the previous frame. */
        if (curFid >= 0 && fid != static_cast<unsigned char>(curFid) && !acc.isEmpty()) {
            const int len = acc.size();
            const unsigned char *a = reinterpret_cast<const unsigned char *>(acc.constData());
            const bool valid = len > 4 &&
                               a[0] == 0xFF && a[1] == 0xD8 &&
                               a[len - 2] == 0xFF && a[len - 1] == 0xD9;
            ++emitted;
            if (emitted == upp::WARMUP_FRAMES + 1)
                qInfo() << "pipecam: first frame," << len << "bytes, jpeg" << (valid ? "valid" : "INVALID");
            if (valid && emitted > upp::WARMUP_FRAMES) {
                QImage img = QImage::fromData(acc, "JPEG");
                if (!img.isNull()) {
                    applyGain(&img);
                    /* The QImage carries the brightened picture; `acc` stays
                     * the camera's untouched JPEG, so recording remains a
                     * lossless mux. */
                    emit frameReady(img, acc);
                }
            }
            acc.clear();
        }

        if (chunkLen > 0) {
            if (acc.size() + chunkLen <= upp::MAX_FRAME_BYTES)
                acc.append(packet.constData() + upp::PAYLOAD_OFFSET, chunkLen);
            else
                acc.clear();        /* desynchronised — resynchronise on next fid */
        }
        curFid = fid;
    }
}

void UppCameraWorker::run()
{
    libusb_context *ctx = 0;
    if (libusb_init(&ctx) < 0) {
        emit statusChanged(UppCamera::Error, tr("USB subsystem unavailable."));
        return;
    }
    libusb_set_log_cb(ctx, libusbLog, LIBUSB_LOG_CB_CONTEXT);

    /* What was last reported, so a camera that fails the same way once a
     * second for an hour leaves one log line, not 3600. */
    QString lastLogged;

    /* Supervision loop: the 10 m cable will be unplugged, kinked and pulled.
     * Treat a lost camera as a normal state to recover from, not a failure. */
    while (!m_stop.loadAcquire()) {
        libusb_device_handle *h = 0;
        QString err;

        emit statusChanged(UppCamera::Searching, QString());
        setLibusbVerbose(ctx, DiagLog::isVerbose());
        int rc = openDevice(ctx, &h, &err);
        if (rc != 0) {
            if (err != lastLogged) {
                qInfo() << "pipecam: open:" << libusb_error_name(rc) << "-" << err;
                lastLogged = err;
            }
            emit statusChanged(rc == LIBUSB_ERROR_ACCESS ? UppCamera::Error
                                                         : UppCamera::Searching, err);
            /* Sleep in slices so a stop request is still honoured promptly. */
            for (int i = 0; i < RETRY_DELAY_MS / 100 && !m_stop.loadAcquire(); ++i)
                msleep(100);
            continue;
        }

        emit statusChanged(UppCamera::Connecting, QString());
        emit deviceInfoReady(collectDeviceInfo(h));
        rc = handshake(h, &err);
        if (rc != 0) {
            if (err != lastLogged) {
                qWarning() << "pipecam: handshake failed:" << err;
                lastLogged = err;
            }
            emit statusChanged(UppCamera::Error, err);
            libusb_close(h);
            for (int i = 0; i < RETRY_DELAY_MS / 100 && !m_stop.loadAcquire(); ++i)
                msleep(100);
            continue;
        }

        /* The device needs a moment after CONNECT before the stream is coherent. */
        msleep(300);
        qInfo() << "pipecam: handshake ok, streaming";
        lastLogged.clear();
        setLibusbVerbose(ctx, false);
        emit statusChanged(UppCamera::Streaming, QString());
        streamLoop(h);

        /* Gentle teardown. A libusb_reset_device() here would force a USB
         * re-enumeration and make the NEXT open race against it — which shows
         * up as an intermittent "camera is busy" on restart. */
        libusb_set_interface_alt_setting(h, upp::IFACE_STREAM, 0);
        libusb_release_interface(h, upp::IFACE_STREAM);
        libusb_release_interface(h, upp::IFACE_IAP);
        libusb_close(h);

        if (m_lastButton) {
            m_lastButton = false;
            emit buttonChanged(false);
        }
    }

    libusb_exit(ctx);
    emit statusChanged(UppCamera::Idle, QString());
}

/* =========================================================================
 * UppCamera
 * ========================================================================= */

UppCamera::UppCamera(QObject *parent)
    : QObject(parent)
    , m_worker(0)
    , m_status(Idle)
    , m_frameCount(0)
    , m_fps(0.0)
    , m_fpsFrames(0)
    , m_buttonPressed(false)
    , m_gain(1.0)
{
}

qreal UppCamera::maxGain() const { return MAX_GAIN; }

void UppCamera::setGain(qreal gain)
{
    if (gain < 1.0)
        gain = 1.0;
    if (gain > MAX_GAIN)
        gain = MAX_GAIN;
    if (qFuzzyCompare(m_gain, gain))
        return;
    m_gain = gain;
    /* The worker may not exist yet (camera stopped); start() re-applies it. */
    if (m_worker)
        m_worker->setGainPercent(qRound(m_gain * 100.0));
    emit gainChanged();
}

UppCamera::~UppCamera()
{
    stop();
}

int UppCamera::frameWidth() const  { return upp::FRAME_WIDTH; }
int UppCamera::frameHeight() const { return upp::FRAME_HEIGHT; }

QString UppCamera::statusText() const
{
    switch (m_status) {
    case Idle:       return tr("Off");
    case Searching:  return tr("Looking for camera…");
    case Connecting: return tr("Connecting…");
    case Streaming:  return tr("Live");
    case Error:      return tr("Problem");
    }
    return QString();
}

void UppCamera::start()
{
    if (m_worker)
        return;

    m_worker = new UppCameraWorker(this);
    /* Queued by default (different threads), which is what we want: the GUI
     * thread only ever sees fully-formed frames. */
    connect(m_worker, SIGNAL(frameReady(QImage,QByteArray)),
            this, SLOT(onFrameReady(QImage,QByteArray)));
    connect(m_worker, SIGNAL(statusChanged(int,QString)),
            this, SLOT(onStatusChanged(int,QString)));
    connect(m_worker, SIGNAL(buttonChanged(bool)),
            this, SLOT(onButtonChanged(bool)));
    connect(m_worker, SIGNAL(deviceInfoReady(QVariantMap)),
            this, SLOT(onDeviceInfoReady(QVariantMap)));

    m_frameCount = 0;
    m_fpsFrames = 0;
    m_fps = 0.0;
    m_fpsTimer.start();

    /* Carry the current gain into the new worker, so restarting the camera does
     * not silently reset the brightness the user set. */
    m_worker->setGainPercent(qRound(m_gain * 100.0));

    m_worker->start();
    emit runningChanged();
    emit fpsChanged();
}

void UppCamera::stop()
{
    if (!m_worker)
        return;

    UppCameraWorker *w = m_worker;
    m_worker = 0;                   /* running() is false from here on */
    w->requestStop();
    /* Bulk reads time out after READ_TIMEOUT_MS, so the thread unwinds within
     * about a second; allow generous headroom before giving up on it. */
    if (!w->wait(3000))
        qWarning() << "pipecam: camera thread did not stop in time";
    delete w;

    setStatus(Idle, QString());
    m_fps = 0.0;
    emit fpsChanged();
    emit runningChanged();
}

void UppCamera::setStatus(Status s, const QString &detail)
{
    if (m_status == s && m_statusDetail == detail)
        return;
    m_status = s;
    m_statusDetail = detail;
    emit statusChanged();
}

void UppCamera::onFrameReady(const QImage &image, const QByteArray &jpeg)
{
    /* A frame can still be in flight when stop() runs; drop it rather than
     * showing a stale picture after the user switched the camera off. */
    if (!m_worker)
        return;

    m_image = image;
    m_jpeg = jpeg;
    ++m_frameCount;

    ++m_fpsFrames;
    const qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {
        const qreal newFps = m_fpsFrames * 1000.0 / qreal(elapsed);
        m_fpsFrames = 0;
        m_fpsTimer.restart();
        if (!qFuzzyCompare(newFps, m_fps)) {
            m_fps = newFps;
            emit fpsChanged();
        }
    }

    emit frameAvailable();
}

void UppCamera::onStatusChanged(int status, const QString &detail)
{
    if (!m_worker && status != Idle)
        return;
    setStatus(static_cast<Status>(status), detail);
}

void UppCamera::onDeviceInfoReady(const QVariantMap &info)
{
    if (info.isEmpty() || m_deviceInfo == info)
        return;
    m_deviceInfo = info;
    emit deviceInfoChanged();
}

void UppCamera::onButtonChanged(bool pressed)
{
    if (m_buttonPressed == pressed)
        return;
    m_buttonPressed = pressed;
    emit buttonPressedChanged();
    /* Fire on the rising edge only, so one press is one action. */
    if (pressed)
        emit buttonClicked();
}
