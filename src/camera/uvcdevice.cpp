/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "uvcdevice.h"
#include "uppprotocol.h"
#include "diaglog.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>

namespace uvc {

static const int WANT_W = 640;
static const int WANT_H = 480;
static const int BUF_COUNT = 4;

static int xioctl(int fd, unsigned long req, void *arg)
{
    int r;
    do { r = ioctl(fd, req, arg); } while (r == -1 && errno == EINTR);
    return r;
}

static QString errnoText(int e)
{
    return QString::fromLocal8Bit(strerror(e));
}

static QString fourcc(unsigned f)
{
    QString s;
    for (int i = 0; i < 4; ++i) {
        const char c = char((f >> (8 * i)) & 0xFF);
        s += (c >= 32 && c < 127) ? QChar::fromLatin1(c) : QChar('?');
    }
    return s;
}

QString sysAttr(const QString &dir, const char *attr)
{
    QFile f(dir + QLatin1Char('/') + QLatin1String(attr));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromLocal8Bit(f.readAll()).trimmed();
}

static QString driverOf(const QString &dir)
{
    const QFileInfo drv(dir + QStringLiteral("/driver"));
    return drv.exists() ? QFileInfo(drv.symLinkTarget()).fileName() : QString();
}

static bool isUppId(int vid, int pid)
{
    for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i)
        if (vid == upp::KNOWN_DEVICES[i].vid && pid == upp::KNOWN_DEVICES[i].pid)
            return true;
    return false;
}

QList<UsbDevice> usbDevices()
{
    QList<UsbDevice> out;
    QSet<QString> seen;
    const QDir usb(QStringLiteral("/sys/bus/usb/devices"));
    const QStringList entries = usb.entryList(QDir::Dirs | QDir::System | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &e : entries) {
        const int colon = e.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        const QString itf = usb.absoluteFilePath(e);
        /* 0e/01: video control */
        if (sysAttr(itf, "bInterfaceClass") != QLatin1String("0e")
                || sysAttr(itf, "bInterfaceSubClass") != QLatin1String("01"))
            continue;
        const QString name = e.left(colon);
        if (seen.contains(name))
            continue;
        seen.insert(name);
        UsbDevice d;
        d.sysName = name;
        d.sysDir = QFileInfo(usb.absoluteFilePath(name)).canonicalFilePath();
        d.vid = sysAttr(d.sysDir, "idVendor").toInt(0, 16);
        d.pid = sysAttr(d.sysDir, "idProduct").toInt(0, 16);
        if (d.sysDir.isEmpty() || isUppId(d.vid, d.pid))
            continue;
        out << d;
    }
    return out;
}

static QStringList nodesUnder(const UsbDevice &d, const char *cls, const char *prefix, const char *devDir)
{
    QStringList out;
    const QDir dir(QStringLiteral("/sys/class/") + QLatin1String(cls));
    const QStringList entries = dir.entryList(QStringList() << QLatin1String(prefix) + QLatin1Char('*'),
                                              QDir::Dirs | QDir::System | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &e : entries) {
        const QString real = QFileInfo(dir.absoluteFilePath(e)).canonicalFilePath();
        if (real.startsWith(d.sysDir + QLatin1Char('/')))
            out << QLatin1String(devDir) + e;
    }
    return out;
}

QStringList videoNodes(const UsbDevice &d)
{
    return nodesUnder(d, "video4linux", "video", "/dev/");
}

QStringList inputNodes(const UsbDevice &d)
{
    return nodesUnder(d, "input", "event", "/dev/input/");
}

static QString controlDriver(const UsbDevice &d)
{
    const QDir dir(d.sysDir);
    const QStringList itfs = dir.entryList(QStringList() << d.sysName + QStringLiteral(":*"),
                                           QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &i : itfs) {
        const QString p = dir.absoluteFilePath(i);
        if (sysAttr(p, "bInterfaceClass") == QLatin1String("0e")
                && sysAttr(p, "bInterfaceSubClass") == QLatin1String("01"))
            return driverOf(p);
    }
    return QString();
}

ScanResult scan(Candidate *c, UserText *err)
{
    ScanResult best = None;
    const QList<UsbDevice> devs = usbDevices();
    for (const UsbDevice &d : devs) {
        const QStringList nodes = videoNodes(d);
        if (nodes.isEmpty()) {
            const QString drv = controlDriver(d);
            /* uvcvideo bound, node not yet created by udev: wait */
            if (drv == QLatin1String("uvcvideo"))
                continue;
            if (best == None) {
                best = NoNode;
                *err = UserText("QObject", QT_TRANSLATE_NOOP("QObject", "UVC camera %1 found, but no video device "
                                                                        "appeared (kernel driver: %2). Please send a "
                                                                        "diagnostic report (Settings → About)."))
                        .arg(QString::asprintf("%04x:%04x", d.vid, d.pid))
                        .arg(drv.isEmpty() ? QStringLiteral("none") : drv);
            }
            continue;
        }
        for (const QString &n : nodes) {
            const QByteArray path = QFile::encodeName(n);
            const int fd = ::open(path.constData(), O_RDWR | O_NONBLOCK);
            if (fd < 0) {
                const int e = errno;
                if (best != Found && (e == EACCES || e == EPERM)) {
                    best = NoAccess;
                    *err = UserText("QObject", QT_TRANSLATE_NOOP("QObject", "UVC camera found but not accessible "
                                                                            "(%1). The permission rule is missing — "
                                                                            "reinstall the app and replug the camera."))
                            .arg(n);
                }
                continue;
            }
            struct v4l2_capability cap;
            memset(&cap, 0, sizeof cap);
            bool usable = false;
            if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
                const unsigned caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps
                                                                                : cap.capabilities;
                usable = (caps & V4L2_CAP_VIDEO_CAPTURE) && (caps & V4L2_CAP_STREAMING)
                        && qstrcmp(reinterpret_cast<const char *>(cap.driver), "uvcvideo") == 0;
            }
            ::close(fd);
            if (!usable)
                continue;
            c->usb = d;
            c->node = n;
            c->card = QString::fromUtf8(reinterpret_cast<const char *>(cap.card));
            return Found;
        }
    }
    return best;
}

Stream::Stream()
    : m_fd(-1)
    , m_evFd(-1)
    , m_pixfmt(0)
    , m_width(0)
    , m_height(0)
    , m_stride(0)
    , m_nbuf(0)
    , m_streaming(false)
    , m_dequeued(-1)
    , m_data(0)
    , m_bytes(0)
    , m_badFrames(0)
{
}

Stream::~Stream()
{
    close();
}

bool Stream::isMjpeg() const
{
    return m_pixfmt == V4L2_PIX_FMT_MJPEG;
}

QString Stream::formatName() const
{
    return isMjpeg() ? QStringLiteral("UVC MJPEG") : QStringLiteral("UVC YUYV 4:2:2");
}

/* Discrete sizes: 640x480, else closest area, smaller on a tie. */
static void pickSize(const QVector<QPair<int, int> > &sizes, int *w, int *h)
{
    *w = WANT_W;
    *h = WANT_H;
    long best = -1;
    for (const QPair<int, int> &s : sizes) {
        if (s.first == WANT_W && s.second == WANT_H) {
            *w = WANT_W;
            *h = WANT_H;
            return;
        }
        const long a = long(s.first) * s.second;
        const long d = a > WANT_W * WANT_H ? 2 * (a - WANT_W * WANT_H) : WANT_W * WANT_H - a;
        if (best < 0 || d < best) {
            best = d;
            *w = s.first;
            *h = s.second;
        }
    }
}

bool Stream::open(const Candidate &c, UserText *err)
{
    close();
    const QByteArray path = QFile::encodeName(c.node);
    m_fd = ::open(path.constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        const int e = errno;
        *err = UserText("QObject", QT_TRANSLATE_NOOP("QObject", "Could not open the UVC camera (%1)."))
                .arg(c.node) + QStringLiteral(" [%1]").arg(errnoText(e));
        return false;
    }

    /* formats and discrete sizes */
    bool haveYuyv = false, haveMjpeg = false;
    QVector<QPair<int, int> > yuyvSizes, mjpegSizes;
    QStringList offered;
    for (unsigned i = 0; i < 32; ++i) {
        struct v4l2_fmtdesc fd;
        memset(&fd, 0, sizeof fd);
        fd.index = i;
        fd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(m_fd, VIDIOC_ENUM_FMT, &fd) != 0)
            break;
        QVector<QPair<int, int> > sizes;
        QStringList sz;
        for (unsigned k = 0; k < 64; ++k) {
            struct v4l2_frmsizeenum fs;
            memset(&fs, 0, sizeof fs);
            fs.index = k;
            fs.pixel_format = fd.pixelformat;
            if (xioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &fs) != 0)
                break;
            if (fs.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                sizes << qMakePair(int(fs.discrete.width), int(fs.discrete.height));
                sz << QStringLiteral("%1x%2").arg(fs.discrete.width).arg(fs.discrete.height);
            } else {
                sz << QStringLiteral("%1x%2..%3x%4").arg(fs.stepwise.min_width).arg(fs.stepwise.min_height)
                      .arg(fs.stepwise.max_width).arg(fs.stepwise.max_height);
                break;
            }
        }
        offered << QStringLiteral("%1 (%2): %3").arg(fourcc(fd.pixelformat))
                   .arg(QString::fromUtf8(reinterpret_cast<const char *>(fd.description)))
                   .arg(sz.join(QLatin1Char(' ')));
        if (fd.pixelformat == V4L2_PIX_FMT_YUYV) {
            haveYuyv = true;
            yuyvSizes = sizes;
        } else if (fd.pixelformat == V4L2_PIX_FMT_MJPEG) {
            haveMjpeg = true;
            mjpegSizes = sizes;
        }
    }
    m_offered = offered.join(QLatin1Char('\n'));
    PIPECAM_TRACE(QStringLiteral("uvc %1 \"%2\" formats:\n%3").arg(c.node, c.card, m_offered));

    if (!haveYuyv && !haveMjpeg) {
        *err = UserText("QObject", QT_TRANSLATE_NOOP("QObject", "The UVC camera offers neither YUYV nor MJPEG."));
        close();
        return false;
    }
    const unsigned want = haveYuyv ? V4L2_PIX_FMT_YUYV : V4L2_PIX_FMT_MJPEG;
    int w, h;
    pickSize(haveYuyv ? yuyvSizes : mjpegSizes, &w, &h);

    UserText setupErr = UserText("QObject", QT_TRANSLATE_NOOP("QObject", "Could not start the UVC video stream."));

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = w;
    fmt.fmt.pix.height = h;
    fmt.fmt.pix.pixelformat = want;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (xioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        *err = setupErr + QStringLiteral(" [S_FMT: %1]").arg(errnoText(errno));
        close();
        return false;
    }
    m_pixfmt = fmt.fmt.pix.pixelformat;
    m_width = int(fmt.fmt.pix.width);
    m_height = int(fmt.fmt.pix.height);
    m_stride = int(fmt.fmt.pix.bytesperline);
    if (m_pixfmt != want || m_width <= 0 || m_height <= 0) {
        *err = setupErr + QStringLiteral(" [S_FMT: got %1 %2x%3]").arg(fourcc(m_pixfmt)).arg(m_width).arg(m_height);
        close();
        return false;
    }
    if (!isMjpeg() && m_stride < 2 * m_width)
        m_stride = 2 * m_width;
    PIPECAM_TRACE(QStringLiteral("uvc S_FMT: %1 %2x%3, bytesperline %4, sizeimage %5")
                  .arg(fourcc(m_pixfmt)).arg(m_width).arg(m_height).arg(m_stride)
                  .arg(fmt.fmt.pix.sizeimage));

    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_G_PARM, &parm) == 0 && parm.parm.capture.timeperframe.numerator)
        PIPECAM_TRACE(QStringLiteral("uvc frame interval: %1/%2 s")
                      .arg(parm.parm.capture.timeperframe.numerator)
                      .arg(parm.parm.capture.timeperframe.denominator));

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof req);
    req.count = BUF_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(m_fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
        *err = setupErr + QStringLiteral(" [REQBUFS: %1, count %2]").arg(errnoText(errno)).arg(req.count);
        close();
        return false;
    }
    const int nbuf = req.count > 8 ? 8 : int(req.count);
    for (int i = 0; i < nbuf; ++i) {
        struct v4l2_buffer b;
        memset(&b, 0, sizeof b);
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        b.index = i;
        if (xioctl(m_fd, VIDIOC_QUERYBUF, &b) < 0) {
            *err = setupErr + QStringLiteral(" [QUERYBUF: %1]").arg(errnoText(errno));
            close();
            return false;
        }
        void *p = mmap(0, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, b.m.offset);
        if (p == MAP_FAILED) {
            *err = setupErr + QStringLiteral(" [mmap: %1]").arg(errnoText(errno));
            close();
            return false;
        }
        m_bufs[i].start = p;
        m_bufs[i].len = b.length;
        m_nbuf = i + 1;
        if (xioctl(m_fd, VIDIOC_QBUF, &b) < 0) {
            *err = setupErr + QStringLiteral(" [QBUF: %1]").arg(errnoText(errno));
            close();
            return false;
        }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        *err = setupErr + QStringLiteral(" [STREAMON: %1]").arg(errnoText(errno));
        close();
        return false;
    }
    m_streaming = true;

    /* button: input device below the same USB device, read only, no grab */
    const QStringList ev = inputNodes(c.usb);
    for (const QString &n : ev) {
        const QByteArray p = QFile::encodeName(n);
        const int fd = ::open(p.constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            PIPECAM_TRACE(QStringLiteral("uvc button %1: %2").arg(n, errnoText(errno)));
            continue;
        }
        unsigned long bits[KEY_MAX / (8 * sizeof(unsigned long)) + 1];
        memset(bits, 0, sizeof bits);
        const bool hasKey = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof bits), bits) >= 0
                && (bits[KEY_CAMERA / (8 * sizeof(unsigned long))]
                    >> (KEY_CAMERA % (8 * sizeof(unsigned long)))) & 1;
        if (!hasKey) {
            ::close(fd);
            continue;
        }
        m_evFd = fd;
        m_buttonNode = n;
        break;
    }
    PIPECAM_TRACE(QStringLiteral("uvc button input: %1 (%2 input node(s) under the device)")
                  .arg(m_buttonNode.isEmpty() ? QStringLiteral("none") : m_buttonNode).arg(ev.size()));
    return true;
}

void Stream::close()
{
    if (m_fd >= 0 && m_streaming) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(m_fd, VIDIOC_STREAMOFF, &type);
    }
    m_streaming = false;
    for (int i = 0; i < m_nbuf; ++i)
        munmap(m_bufs[i].start, m_bufs[i].len);
    m_nbuf = 0;
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
    if (m_evFd >= 0)
        ::close(m_evFd);
    m_evFd = -1;
    m_buttonNode.clear();
    m_dequeued = -1;
    m_data = 0;
    m_bytes = 0;
    if (m_badFrames)
        qInfo() << "pipecam: uvc:" << m_badFrames << "incomplete frames skipped";
    m_badFrames = 0;
}

void Stream::requeue()
{
    if (m_dequeued < 0 || m_fd < 0)
        return;
    struct v4l2_buffer b;
    memset(&b, 0, sizeof b);
    b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    b.memory = V4L2_MEMORY_MMAP;
    b.index = unsigned(m_dequeued);
    xioctl(m_fd, VIDIOC_QBUF, &b);
    m_dequeued = -1;
    m_data = 0;
    m_bytes = 0;
}

void Stream::readButton(QVector<bool> *buttons)
{
    struct input_event ev[16];
    for (;;) {
        const ssize_t n = ::read(m_evFd, ev, sizeof ev);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno != EAGAIN) {
                ::close(m_evFd);
                m_evFd = -1;
            }
            return;
        }
        if (n == 0)
            return;
        for (int i = 0; i < int(n / sizeof(struct input_event)); ++i) {
            if (ev[i].type != EV_KEY)
                continue;
            PIPECAM_TRACE(QStringLiteral("uvc key %1 = %2").arg(ev[i].code).arg(ev[i].value));
            /* 2 = autorepeat */
            if (ev[i].code == KEY_CAMERA && ev[i].value != 2)
                buttons->append(ev[i].value != 0);
        }
    }
}

Stream::Result Stream::next(int timeoutMs, QVector<bool> *buttons)
{
    requeue();
    if (m_fd < 0)
        return Gone;
    QElapsedTimer t;
    t.start();
    for (;;) {
        const int left = timeoutMs - int(t.elapsed());
        if (left <= 0)
            return Timeout;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        int maxFd = m_fd;
        if (m_evFd >= 0) {
            FD_SET(m_evFd, &fds);
            if (m_evFd > maxFd)
                maxFd = m_evFd;
        }
        struct timeval tv;
        tv.tv_sec = left / 1000;
        tv.tv_usec = (left % 1000) * 1000;
        const int r = select(maxFd + 1, &fds, 0, 0, &tv);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            qWarning() << "pipecam: uvc select:" << errnoText(errno);
            return Gone;
        }
        if (r == 0)
            return Timeout;
        if (m_evFd >= 0 && FD_ISSET(m_evFd, &fds))
            readButton(buttons);
        if (!FD_ISSET(m_fd, &fds))
            continue;

        struct v4l2_buffer b;
        memset(&b, 0, sizeof b);
        b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        b.memory = V4L2_MEMORY_MMAP;
        if (xioctl(m_fd, VIDIOC_DQBUF, &b) < 0) {
            if (errno == EAGAIN)
                continue;
            qInfo() << "pipecam: uvc DQBUF:" << errnoText(errno);
            return Gone;
        }
        if (int(b.index) >= m_nbuf) {
            xioctl(m_fd, VIDIOC_QBUF, &b);
            continue;
        }
        const int need = isMjpeg() ? 4 : m_stride * m_height;
        if ((b.flags & V4L2_BUF_FLAG_ERROR) || int(b.bytesused) < need
                || b.bytesused > m_bufs[b.index].len) {
            if (++m_badFrames <= 5)
                PIPECAM_TRACE(QStringLiteral("uvc frame skipped: %1 bytes, flags 0x%2")
                              .arg(b.bytesused).arg(b.flags, 0, 16));
            xioctl(m_fd, VIDIOC_QBUF, &b);
            continue;
        }
        m_dequeued = int(b.index);
        m_data = static_cast<const unsigned char *>(m_bufs[b.index].start);
        m_bytes = int(b.bytesused);
        return Frame;
    }
}

static QString speedText(const QString &s)
{
    if (s == QLatin1String("1.5"))  return QStringLiteral("1.5 Mbit/s (low)");
    if (s == QLatin1String("12"))   return QStringLiteral("12 Mbit/s (full)");
    if (s == QLatin1String("480"))  return QStringLiteral("480 Mbit/s (high)");
    if (s == QLatin1String("5000")) return QStringLiteral("5 Gbit/s (super)");
    return s.isEmpty() ? QStringLiteral("unknown") : s + QStringLiteral(" Mbit/s");
}

QVariantMap Stream::deviceInfo(const Candidate &c) const
{
    const QString d = c.usb.sysDir;
    QVariantMap info;
    info["transport"]    = QStringLiteral("uvc");
    info["manufacturer"] = sysAttr(d, "manufacturer");
    info["product"]      = sysAttr(d, "product").isEmpty() ? c.card : sysAttr(d, "product");
    info["serial"]       = sysAttr(d, "serial");
    info["vendorId"]     = QString::asprintf("%04x", c.usb.vid);
    info["productId"]    = QString::asprintf("%04x", c.usb.pid);
    info["usbVersion"]   = sysAttr(d, "version");
    const int bcd = sysAttr(d, "bcdDevice").toInt(0, 16);
    info["deviceVersion"] = QStringLiteral("%1.%2").arg(bcd >> 8, 0, 16).arg((bcd >> 4) & 0xF, 0, 16);
    info["busNumber"]    = sysAttr(d, "busnum").toInt();
    info["deviceAddress"] = sysAttr(d, "devnum").toInt();
    info["speed"]        = speedText(sysAttr(d, "speed"));
    info["deviceClass"]  = sysAttr(d, "bDeviceClass");
    QString power = sysAttr(d, "bMaxPower");
    power.replace(QStringLiteral("mA"), QStringLiteral(" mA"));
    info["maxPower"]     = power;

    QStringList ifaces;
    const QDir dir(d);
    const QStringList itfs = dir.entryList(QStringList() << c.usb.sysName + QStringLiteral(":*"),
                                           QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &i : itfs) {
        const QString p = dir.absoluteFilePath(i);
        const QString drv = driverOf(p);
        ifaces << QStringLiteral("%1: class %2/%3/%4%5")
                  .arg(sysAttr(p, "bInterfaceNumber").toInt(0, 16))
                  .arg(sysAttr(p, "bInterfaceClass"), sysAttr(p, "bInterfaceSubClass"),
                       sysAttr(p, "bInterfaceProtocol"))
                  .arg(drv.isEmpty() ? QString() : QStringLiteral(", ") + drv);
    }
    info["interfaces"]   = ifaces.join(QLatin1String("\n"));
    info["format"]       = formatName();
    info["videoNode"]    = c.node;
    info["offered"]      = m_offered;
    info["buttonNode"]   = m_buttonNode;
    return info;
}

bool jpegHasDht(const unsigned char *p, int n)
{
    if (n < 4 || p[0] != 0xFF || p[1] != 0xD8)
        return false;
    int i = 2;
    while (i + 4 <= n) {
        if (p[i] != 0xFF)
            return false;
        const unsigned char m = p[i + 1];
        if (m == 0xFF) {
            ++i;
            continue;
        }
        if (m == 0xC4)
            return true;
        if (m == 0xDA || m == 0xD9)
            return false;
        if ((m >= 0xD0 && m <= 0xD7) || m == 0x01) {
            i += 2;
            continue;
        }
        i += 2 + ((int(p[i + 2]) << 8) | p[i + 3]);
    }
    return false;
}

} // namespace uvc
