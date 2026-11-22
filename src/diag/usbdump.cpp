/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "usbdump.h"
#include "uppprotocol.h"
#include "uppvariant.h"
#include "uvcdevice.h"

#include <libusb-1.0/libusb.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace usbdump {

static QString hex(int v, int width)
{
    return QStringLiteral("%1").arg(v, width, 16, QChar('0'));
}

static QString bcd(uint16_t v)
{
    return QStringLiteral("%1.%2").arg(v >> 8, 0, 16).arg(v & 0xFF, 2, 16, QChar('0'));
}

static QString readSys(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QStringLiteral("-");
    return QString::fromLocal8Bit(f.readAll()).trimmed();
}

static bool isCamera(const libusb_device_descriptor &d)
{
    for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i)
        if (d.idVendor == upp::KNOWN_DEVICES[i].vid && d.idProduct == upp::KNOWN_DEVICES[i].pid)
            return true;
    return false;
}

static bool isUvc(libusb_device *dev, const libusb_device_descriptor &d)
{
    if (isCamera(d))
        return false;
    libusb_config_descriptor *cfg = 0;
    if (libusb_get_active_config_descriptor(dev, &cfg) != 0 || !cfg)
        return false;
    bool uvc = false;
    for (int i = 0; i < cfg->bNumInterfaces && !uvc; ++i) {
        const libusb_interface &itf = cfg->interface[i];
        uvc = itf.num_altsetting > 0 && itf.altsetting[0].bInterfaceClass == 0x0e;
    }
    libusb_free_config_descriptor(cfg);
    return uvc;
}

/* e.g. "1-1.2" under /sys/bus/usb/devices */
static QString sysName(libusb_device *dev)
{
    uint8_t ports[8];
    const int n = libusb_get_port_numbers(dev, ports, sizeof(ports));
    if (n <= 0)
        return QStringLiteral("usb%1").arg(libusb_get_bus_number(dev));
    QString s = QStringLiteral("%1-").arg(libusb_get_bus_number(dev));
    for (int i = 0; i < n; ++i)
        s += (i ? QStringLiteral(".") : QString()) + QString::number(ports[i]);
    return s;
}

static QString speedName(int s)
{
    switch (s) {
    case LIBUSB_SPEED_LOW:   return QStringLiteral("low (1.5 Mbit/s)");
    case LIBUSB_SPEED_FULL:  return QStringLiteral("full (12 Mbit/s)");
    case LIBUSB_SPEED_HIGH:  return QStringLiteral("high (480 Mbit/s)");
    case LIBUSB_SPEED_SUPER: return QStringLiteral("super (5 Gbit/s)");
    default:                 return QStringLiteral("unknown");
    }
}

static QString epType(uint8_t attr)
{
    switch (attr & 0x3) {
    case LIBUSB_TRANSFER_TYPE_CONTROL:     return QStringLiteral("control");
    case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS: return QStringLiteral("isoc");
    case LIBUSB_TRANSFER_TYPE_BULK:        return QStringLiteral("bulk");
    default:                               return QStringLiteral("interrupt");
    }
}

static QString nodeInfo(const QString &node)
{
    struct stat st;
    if (::stat(QFile::encodeName(node).constData(), &st) != 0)
        return QStringLiteral("%1: missing").arg(node);
    const struct passwd *pw = ::getpwuid(st.st_uid);
    const struct group *gr = ::getgrgid(st.st_gid);
    return QStringLiteral("%1: mode %2 owner %3:%4, readable=%5 writable=%6")
            .arg(node)
            .arg(st.st_mode & 07777, 4, 8, QChar('0'))
            .arg(pw ? QString::fromLocal8Bit(pw->pw_name) : QString::number(st.st_uid))
            .arg(gr ? QString::fromLocal8Bit(gr->gr_name) : QString::number(st.st_gid))
            .arg(::access(QFile::encodeName(node).constData(), R_OK) == 0 ? "yes" : "no")
            .arg(::access(QFile::encodeName(node).constData(), W_OK) == 0 ? "yes" : "no");
}

static QString stringDesc(libusb_device_handle *h, uint8_t idx, QStringList *collect)
{
    if (!h || idx == 0)
        return QString();
    unsigned char buf[256];
    const int n = libusb_get_string_descriptor_ascii(h, idx, buf, sizeof(buf));
    if (n <= 0)
        return QStringLiteral("(%1)").arg(QString::fromLatin1(libusb_error_name(n)));
    const QString s = QString::fromLatin1(reinterpret_cast<char *>(buf), n);
    if (collect)
        *collect << s;
    return QStringLiteral("\"%1\"").arg(s);
}

static QString hexBytes(const unsigned char *p, int n)
{
    QString s;
    for (int i = 0; i < n; ++i)
        s += QString::asprintf("%02x ", p[i]);
    return s.trimmed();
}

/* Class-specific VideoStreaming descriptors (UVC 1.1, 3.9). */
static void dumpUvcExtra(QTextStream &o, const unsigned char *p, int n)
{
    int i = 0;
    while (i + 3 <= n) {
        const int len = p[i];
        if (len < 3 || i + len > n)
            break;
        const unsigned char *q = p + i;
        if (q[1] == 0x24) {
            switch (q[2]) {
            case 0x01:  /* VS_INPUT_HEADER */
                if (len >= 13)
                    o << "      VS input header: bNumFormats " << int(q[3]) << ", bEndpointAddress 0x"
                      << hex(q[6], 2) << ", bStillCaptureMethod " << int(q[9])
                      << ", bTriggerSupport " << int(q[10]) << ", bTriggerUsage " << int(q[11]) << '\n';
                break;
            case 0x04:  /* VS_FORMAT_UNCOMPRESSED */
                if (len >= 22)
                    o << "      format " << int(q[3]) << ": uncompressed "
                      << QString::fromLatin1(reinterpret_cast<const char *>(q + 5), 4).replace(QChar(0), QChar('.'))
                      << ", " << int(q[21]) << " bit/pixel, " << int(q[4]) << " frame(s)\n";
                break;
            case 0x06:  /* VS_FORMAT_MJPEG */
                if (len >= 5)
                    o << "      format " << int(q[3]) << ": MJPEG, " << int(q[4]) << " frame(s)\n";
                break;
            case 0x05:  /* VS_FRAME_UNCOMPRESSED */
            case 0x07:  /* VS_FRAME_MJPEG */
                if (len >= 26) {
                    const int w = q[5] | (q[6] << 8), h = q[7] | (q[8] << 8);
                    const quint32 iv = quint32(q[21]) | (quint32(q[22]) << 8) | (quint32(q[23]) << 16)
                            | (quint32(q[24]) << 24);
                    o << "        frame " << int(q[3]) << ": " << w << 'x' << h << ", default interval "
                      << iv << " (" << (iv ? QString::number(10000000.0 / iv, 'f', 1) : QStringLiteral("-"))
                      << " fps)\n";
                }
                break;
            default:
                o << "      cs descriptor 0x" << hex(q[2], 2) << ": " << hexBytes(q, len) << '\n';
                break;
            }
        }
        i += len;
    }
}

static void dumpUvcSide(QTextStream &o, libusb_device *dev, Result *r)
{
    QList<uvc::UsbDevice> devs = uvc::usbDevices();
    const QString sys = sysName(dev);
    for (const uvc::UsbDevice &u : devs) {
        if (u.sysName != sys)
            continue;
        const QString serial = uvc::sysAttr(u.sysDir, "serial");
        if (!serial.isEmpty())
            r->serials << serial;
        o << "UVC side:\n"
          << "  uvcvideo driver registered: "
          << (QFile::exists(QStringLiteral("/sys/bus/usb/drivers/uvcvideo")) ? "yes" : "no") << '\n';
        const QStringList video = uvc::videoNodes(u);
        if (video.isEmpty())
            o << "  no video4linux node\n";
        for (const QString &n : video) {
            const QString name = uvc::sysAttr(QStringLiteral("/sys/class/video4linux/")
                                              + QFileInfo(n).fileName(), "name");
            o << "  " << nodeInfo(n) << ", name \"" << name << "\"\n";
            r->nodes << n;
        }
        const QStringList input = uvc::inputNodes(u);
        if (input.isEmpty())
            o << "  no input device (button events need bTriggerSupport=1, bTriggerUsage=1)\n";
        for (const QString &n : input) {
            const QString name = uvc::sysAttr(QStringLiteral("/sys/class/input/")
                                              + QFileInfo(n).fileName() + QStringLiteral("/device"), "name");
            o << "  " << nodeInfo(n) << ", name \"" << name << "\"\n";
        }
    }
}

static void dumpCamera(QTextStream &o, libusb_device *dev, const libusb_device_descriptor &d,
                       bool claimTest, Result *r, bool upp = true)
{
    const QString sys = sysName(dev);
    const QString node = QStringLiteral("/dev/bus/usb/%1/%2")
            .arg(libusb_get_bus_number(dev), 3, 10, QChar('0'))
            .arg(libusb_get_device_address(dev), 3, 10, QChar('0'));
    r->nodes << node;

    o << (upp ? "Camera " : "UVC camera ") << hex(d.idVendor, 4) << ':' << hex(d.idProduct, 4)
      << " at " << sys << ", speed " << speedName(libusb_get_device_speed(dev)) << '\n';
    o << "  " << nodeInfo(node) << '\n';

    libusb_device_handle *h = 0;
    const int orc = libusb_open(dev, &h);
    o << "  open: " << (orc == 0 ? QStringLiteral("ok")
                                 : QString::fromLatin1(libusb_error_name(orc))) << '\n';

    o << "Device descriptor:\n"
      << "  bcdUSB " << bcd(d.bcdUSB) << ", class " << hex(d.bDeviceClass, 2) << '/'
      << hex(d.bDeviceSubClass, 2) << '/' << hex(d.bDeviceProtocol, 2)
      << ", bMaxPacketSize0 " << int(d.bMaxPacketSize0) << '\n'
      << "  bcdDevice " << bcd(d.bcdDevice) << ", bNumConfigurations "
      << int(d.bNumConfigurations) << '\n'
      << "  iManufacturer " << int(d.iManufacturer) << ' ' << stringDesc(h, d.iManufacturer, 0) << '\n'
      << "  iProduct      " << int(d.iProduct) << ' ' << stringDesc(h, d.iProduct, 0) << '\n';
    /* read for redaction only; never printed */
    stringDesc(h, d.iSerialNumber, &r->serials);
    o << "  iSerial       " << int(d.iSerialNumber)
      << (d.iSerialNumber ? " <removed>" : "") << '\n';

    for (int c = 0; c < d.bNumConfigurations; ++c) {
        libusb_config_descriptor *cfg = 0;
        const int crc = libusb_get_config_descriptor(dev, uint8_t(c), &cfg);
        if (crc != 0 || !cfg) {
            o << "  configuration #" << c << ": " << libusb_error_name(crc) << '\n';
            continue;
        }
        o << "  Configuration " << int(cfg->bConfigurationValue)
          << ": wTotalLength " << cfg->wTotalLength
          << ", bNumInterfaces " << int(cfg->bNumInterfaces)
          << ", bmAttributes 0x" << hex(cfg->bmAttributes, 2)
          << ", MaxPower " << cfg->MaxPower * 2 << " mA\n";
        for (int i = 0; i < cfg->bNumInterfaces; ++i) {
            const libusb_interface &itf = cfg->interface[i];
            for (int a = 0; a < itf.num_altsetting; ++a) {
                const libusb_interface_descriptor &id = itf.altsetting[a];
                o << "    Interface " << int(id.bInterfaceNumber) << " alt " << int(id.bAlternateSetting)
                  << ": class " << hex(id.bInterfaceClass, 2) << '/'
                  << hex(id.bInterfaceSubClass, 2) << '/' << hex(id.bInterfaceProtocol, 2)
                  << ", " << int(id.bNumEndpoints) << " endpoint(s)\n";
                for (int e = 0; e < id.bNumEndpoints; ++e) {
                    const libusb_endpoint_descriptor &ep = id.endpoint[e];
                    o << "      EP 0x" << hex(ep.bEndpointAddress, 2)
                      << ((ep.bEndpointAddress & 0x80) ? " IN  " : " OUT ")
                      << epType(ep.bmAttributes) << ", wMaxPacketSize " << ep.wMaxPacketSize
                      << ", bInterval " << int(ep.bInterval) << '\n';
                }
                if (!upp && id.bInterfaceClass == 0x0e && id.bInterfaceSubClass == 0x02 && id.extra_length > 0)
                    dumpUvcExtra(o, id.extra, id.extra_length);
            }
            /* "usbfs" = a libusb user */
            const QString ifDir = QStringLiteral("/sys/bus/usb/devices/%1:%2.%3")
                    .arg(sys).arg(cfg->bConfigurationValue).arg(itf.altsetting[0].bInterfaceNumber);
            const QFileInfo drv(ifDir + QStringLiteral("/driver"));
            o << "      kernel driver (sysfs): "
              << (drv.exists() ? QFileInfo(drv.symLinkTarget()).fileName() : QStringLiteral("none"))
              << '\n';
            if (h) {
                const int k = libusb_kernel_driver_active(h, itf.altsetting[0].bInterfaceNumber);
                o << "      libusb_kernel_driver_active: "
                  << (k == 0 ? QStringLiteral("no") : k == 1 ? QStringLiteral("yes")
                                                 : QString::fromLatin1(libusb_error_name(k)))
                  << '\n';
            }
        }
        libusb_free_config_descriptor(cfg);
    }

    if (h) {
        int cur = -1;
        const int grc = libusb_get_configuration(h, &cur);
        o << "  active configuration: "
          << (grc == 0 ? QString::number(cur) : QString::fromLatin1(libusb_error_name(grc))) << '\n';
    }

    if (!upp) {
        dumpUvcSide(o, dev, r);
        claimTest = false;  /* would detach uvcvideo */
    }

    upp::Variant variant = upp::VariantUnknown;
    if (upp) {
        libusb_config_descriptor *cfg = 0;
        if (libusb_get_active_config_descriptor(dev, &cfg) == 0 && cfg) {
            variant = upp::variantOf(cfg);
            libusb_free_config_descriptor(cfg);
        }
        o << "  variant: " << upp::variantName(variant) << '\n';
    }

    if (h && claimTest) {
        /* mirrors the camera worker's claim sequence */
        libusb_set_auto_detach_kernel_driver(h, 1);
        o << "Claim test (BUSY is expected if a PipeCam listed under holders streams):\n";
        int first = upp::IFACE_IAP, last = upp::IFACE_STREAM;
        if (variant == upp::VariantYuyv)
            first = last = upp::yuyv::IFACE;
        for (int iface = first; iface <= last; ++iface) {
            const int rc = libusb_claim_interface(h, iface);
            o << "  claim interface " << iface << ": "
              << (rc == 0 ? QStringLiteral("ok") : QString::fromLatin1(libusb_error_name(rc))) << '\n';
            if (rc == 0)
                libusb_release_interface(h, iface);
        }
    }

    const QString base = QStringLiteral("/sys/bus/usb/devices/") + sys;
    o << "sysfs " << sys << ":\n";
    static const char *const ATTRS[] = {
        "speed", "version", "bMaxPower", "bConfigurationValue", "authorized",
        "avoid_reset_quirk", "quirks", "removable", "power/control",
        "power/runtime_status", "power/autosuspend_delay_ms", "power/wakeup",
    };
    for (const char *a : ATTRS) {
        if (QFile::exists(base + '/' + a))
            o << "  " << a << ": " << readSys(base + '/' + a) << '\n';
    }

    if (h)
        libusb_close(h);
}

Result dump(bool claimTest)
{
    Result r;
    QTextStream o(&r.text);

    libusb_context *ctx = 0;
    const int irc = libusb_init(&ctx);
    if (irc < 0) {
        o << "libusb_init failed: " << libusb_error_name(irc) << '\n';
        return r;
    }

    libusb_device **list = 0;
    const ssize_t n = libusb_get_device_list(ctx, &list);
    if (n < 0) {
        o << "libusb_get_device_list failed: " << libusb_error_name(int(n)) << '\n';
        libusb_exit(ctx);
        return r;
    }

    o << "All USB devices (" << n << "):\n";
    for (ssize_t i = 0; i < n; ++i) {
        libusb_device_descriptor d;
        if (libusb_get_device_descriptor(list[i], &d) != 0)
            continue;
        /* IDs and class only: other devices' strings may identify the user */
        o << "  " << sysName(list[i]) << "  " << hex(d.idVendor, 4) << ':' << hex(d.idProduct, 4)
          << "  class " << hex(d.bDeviceClass, 2) << "  " << speedName(libusb_get_device_speed(list[i]))
          << (isCamera(d) ? "  <- camera" : isUvc(list[i], d) ? "  <- UVC camera" : "") << '\n';
    }

    int cams = 0;
    for (ssize_t i = 0; i < n; ++i) {
        libusb_device_descriptor d;
        if (libusb_get_device_descriptor(list[i], &d) != 0 || !isCamera(d))
            continue;
        o << '\n';
        dumpCamera(o, list[i], d, claimTest, &r);
        ++cams;
    }
    for (ssize_t i = 0; i < n; ++i) {
        libusb_device_descriptor d;
        if (libusb_get_device_descriptor(list[i], &d) != 0 || !isUvc(list[i], d))
            continue;
        o << '\n';
        dumpCamera(o, list[i], d, false, &r, false);
        ++cams;
    }
    if (!cams)
        o << "\nNo known camera on the bus.\n";

    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    o.flush();
    return r;
}

/* driver sits on the parent; canonicalize first, "usbN/.." is lexical */
static QString controllerDriver(const QString &rootHub)
{
    const QString real = QFileInfo(rootHub).canonicalFilePath();
    if (real.isEmpty())
        return QStringLiteral("-");
    const QFileInfo drv(QFileInfo(real).dir().absoluteFilePath(QStringLiteral("driver")));
    return drv.exists() ? QFileInfo(drv.symLinkTarget()).fileName() : QStringLiteral("none");
}

QString hostSide()
{
    QString s;
    QTextStream o(&s);

    o << "Type-C ports:\n";
    const QDir tc(QStringLiteral("/sys/class/typec"));
    const QStringList ports = tc.entryList(QStringList() << QStringLiteral("port*"), QDir::Dirs);
    if (ports.isEmpty())
        o << "  none in /sys/class/typec\n";
    for (const QString &p : ports) {
        if (p.contains('-'))
            continue;   /* portN-partner */
        const QString b = tc.absoluteFilePath(p);
        o << "  " << p << ": data_role " << readSys(b + "/data_role")
          << ", power_role " << readSys(b + "/power_role")
          << ", power_operation_mode " << readSys(b + "/power_operation_mode")
          << ", partner " << (QFile::exists(b + "-partner") ? "yes" : "no") << '\n';
    }

    o << "Host controllers:\n";
    const QDir usb(QStringLiteral("/sys/bus/usb/devices"));
    for (const QString &d : usb.entryList(QStringList() << QStringLiteral("usb*"), QDir::Dirs | QDir::System)) {
        const QString b = usb.absoluteFilePath(d);
        o << "  " << d << ": " << readSys(b + "/product") << ", speed " << readSys(b + "/speed")
          << ", controller driver " << controllerDriver(b) << '\n';
    }

    o << "udev rules:\n";
    static const char *const RULES[] = {
        "/etc/udev/rules.d/999-harbour-pipecam-usb.rules",
        "/etc/udev/rules.d/99-harbour-pipecam-usb.rules",
        "/lib/udev/rules.d/999-android-system.rules",
        "/usr/lib/udev/rules.d/999-android-system.rules",
    };
    for (const char *r : RULES)
        o << "  " << r << ": " << (QFile::exists(QString::fromLatin1(r)) ? "present" : "absent") << '\n';

    o.flush();
    return s;
}

QString holders(const QStringList &nodes)
{
    QString s;
    QTextStream o(&s);
    int seen = 0;
    /* readdir, not QDir: QDir filters stat() fd links and drop device nodes */
    DIR *proc = ::opendir("/proc");
    while (proc) {
        struct dirent *pe = ::readdir(proc);
        if (!pe)
            break;
        if (pe->d_name[0] < '0' || pe->d_name[0] > '9')
            continue;
        const QString pid = QString::fromLatin1(pe->d_name);
        const QByteArray fdDir = "/proc/" + QByteArray(pe->d_name) + "/fd";
        DIR *fd = ::opendir(fdDir.constData());
        if (!fd)
            continue;   /* other user: needs root */
        while (struct dirent *fe = ::readdir(fd)) {
            if (fe->d_name[0] < '0' || fe->d_name[0] > '9')
                continue;
            char buf[512];
            const QByteArray link = fdDir + "/" + QByteArray(fe->d_name);
            const ssize_t len = ::readlink(link.constData(), buf, sizeof(buf) - 1);
            if (len <= 0)
                continue;
            const QString target = QString::fromLocal8Bit(buf, int(len));
            if (nodes.contains(target)) {
                o << "  pid " << pid << " (" << readSys(QStringLiteral("/proc/%1/comm").arg(pid))
                  << ") holds " << target
                  << (pid.toLongLong() == ::getpid() ? "  <- PipeCam itself" : "") << '\n';
                ++seen;
            }
        }
        ::closedir(fd);
    }
    if (proc)
        ::closedir(proc);
    if (!seen)
        o << "  none visible (without root only this user's processes can be checked)\n";
    o.flush();
    return s;
}

} // namespace usbdump
