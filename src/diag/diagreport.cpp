/*
 * diagreport.cpp — see diagreport.h.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "diagreport.h"
#include "diaglog.h"
#include "redact.h"
#include "rootclient.h"
#include "usbdump.h"

#include <libusb-1.0/libusb.h>
#include <gst/gst.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTextStream>

#include <dirent.h>
#include <sys/utsname.h>
#include <unistd.h>

#define PIPECAM_STR2(x) #x
#define PIPECAM_STR(x) PIPECAM_STR2(x)

DiagReport::DiagReport(QObject *parent)
    : QObject(parent)
{
}

QString DiagReport::appVersion()
{
    return QStringLiteral(PIPECAM_STR(PIPECAM_VERSION));
}

/* KEY=value files such as /etc/os-release, reduced to the keys asked for. */
static QString releaseField(const QString &file, const QString &key)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    for (const QByteArray &raw : f.readAll().split('\n')) {
        const QString l = QString::fromUtf8(raw).trimmed();
        if (l.startsWith(key + '=')) {
            QString v = l.mid(key.size() + 1);
            if (v.size() >= 2 && v.startsWith('"') && v.endsWith('"'))
                v = v.mid(1, v.size() - 2);
            return v;
        }
    }
    return QString();
}

static QString orDash(const QString &s)
{
    return s.isEmpty() ? QStringLiteral("-") : s;
}

/* Process names that belong to Android App Support. The container runs as its
 * own users, but /proc/<pid>/comm is world readable, so no root is needed to
 * see that it is up. */
static QString androidContainer()
{
    QSet<QString> names;
    DIR *proc = ::opendir("/proc");
    while (proc) {
        struct dirent *e = ::readdir(proc);
        if (!e)
            break;
        if (e->d_name[0] < '0' || e->d_name[0] > '9')
            continue;
        QFile f(QStringLiteral("/proc/%1/comm").arg(QString::fromLatin1(e->d_name)));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QString comm = QString::fromLatin1(f.readAll()).trimmed();
        const QString low = comm.toLower();
        if (low.contains(QLatin1String("appsupport")) || low.contains(QLatin1String("alien"))
                || low == QLatin1String("lxc-start") || low == QLatin1String("system_server")
                || low.contains(QLatin1String("android.hardware.usb")))
            names.insert(comm);
    }
    if (proc)
        ::closedir(proc);
    if (names.isEmpty())
        return QStringLiteral("not running");
    QStringList l = names.toList();
    l.sort();
    return QStringLiteral("running (%1)").arg(l.join(QStringLiteral(", ")));
}

static QString gstPlugin(const char *factory)
{
    GstElementFactory *f = gst_element_factory_find(factory);
    if (!f)
        return QStringLiteral("missing");
    gst_object_unref(f);
    return QStringLiteral("present");
}

static void section(QTextStream &o, const char *title)
{
    o << "\n## " << title << "\n\n";
}

QString DiagReport::build(const QVariantMap &appState, bool claimTest, bool useRoot)
{
    QString raw;
    QTextStream o(&raw);

    /* Read the descriptors first: the serial they contain has to be known to
     * the redactor before anything is redacted. */
    const usbdump::Result usb = usbdump::dump(claimTest);

    Redactor red;
    for (const QString &s : usb.serials)
        red.addSecret(s, QStringLiteral("<serial>"));
    const QString infoSerial = appState.value(QStringLiteral("camera.serial")).toString();
    red.addSecret(infoSerial, QStringLiteral("<serial>"));

    o << "# PipeCam diagnostic report\n\n"
      << "Anonymised on the device: serial numbers, host name, user name, home\n"
      << "directory, MAC/IP/e-mail addresses and IMEI-length numbers are removed.\n"
      << "Please read it through once before posting anyway.\n";

    section(o, "App");
    const libusb_version *lv = libusb_get_version();
    gchar *gv = gst_version_string();
    o << "PipeCam:    " << appVersion() << '\n'
      << "Qt:         " << qVersion() << '\n'
      << "libusb:     " << lv->major << '.' << lv->minor << '.' << lv->micro << lv->rc << '\n'
      << "GStreamer:  " << gv << '\n'
      << "  jpegparse " << gstPlugin("jpegparse") << ", qtmux " << gstPlugin("qtmux")
      << ", appsrc " << gstPlugin("appsrc") << '\n'
      << "Verbose log: " << (DiagLog::isVerbose() ? "on" : "off") << '\n';
    g_free(gv);
    o << "State and settings:\n";
    QStringList keys = appState.keys();
    keys.sort();
    for (const QString &k : keys) {
        if (k == QLatin1String("camera.serial"))
            continue;
        o << "  " << k << " = " << appState.value(k).toString() << '\n';
    }

    section(o, "System");
    struct utsname u;
    const bool haveUname = ::uname(&u) == 0;
    o << "OS:         " << orDash(releaseField(QStringLiteral("/etc/os-release"), QStringLiteral("PRETTY_NAME")))
      << " (" << orDash(releaseField(QStringLiteral("/etc/os-release"), QStringLiteral("VERSION_ID"))) << ")\n"
      << "Device:     " << orDash(releaseField(QStringLiteral("/etc/hw-release"), QStringLiteral("NAME")))
      << " [" << orDash(releaseField(QStringLiteral("/etc/hw-release"), QStringLiteral("MER_HA_DEVICE"))) << "]\n"
      /* release and machine only — uname's version field names the kernel's
       * build host and date. */
      << "Kernel:     " << (haveUname ? QString::fromLatin1(u.release) : QStringLiteral("-"))
      << ' ' << (haveUname ? QString::fromLatin1(u.machine) : QString()) << '\n'
      << "App uid:    " << ::getuid() << ", groups:";
    gid_t groups[64];
    const int ng = ::getgroups(64, groups);
    for (int i = 0; i < ng; ++i)
        o << ' ' << groups[i];
    o << '\n'
      << "Android App Support: " << androidContainer() << '\n';

    section(o, "USB");
    o << "```\n" << usb.text << "\n" << usbdump::hostSide()
      << "Processes holding the camera node:\n" << usbdump::holders(usb.nodes) << "```\n";

    section(o, "Root helper");
    RootClient *rc = RootClient::instance();
    if (!useRoot) {
        o << "not used\n";
    } else if (!rc->active()) {
        o << "requested but not connected" << (rc->lastError().isEmpty() ? QString()
                                               : QStringLiteral(": ") + rc->lastError()) << '\n';
    } else {
        struct Part { char cmd; const char *title; int timeout; };
        static const Part PARTS[] = {
            { 'H', "Processes of any user holding the camera", 3000 },
            { 'U', "Kernel USB device table (debugfs), camera only", 3000 },
            { 'L', "lsusb -v", 8000 },
            { 'K', "Kernel log, USB lines", 5000 },
            { 'J', "Journal of this boot, USB/PipeCam lines", 20000 },
        };
        QString rootText;
        QTextStream ro(&rootText);
        for (const Part &p : PARTS) {
            const QByteArray a = rc->request(p.cmd, p.timeout);
            ro << "### " << p.title << "\n```\n"
               << (a.isEmpty() ? QStringLiteral("(no answer)\n") : QString::fromUtf8(a))
               << "```\n";
        }
        ro.flush();
        o << red.log(rootText);
    }

    section(o, "Log of this run");
    const QStringList lines = DiagLog::instance()->lines();
    o << "```\n" << (lines.isEmpty() ? QStringLiteral("(empty)") : red.log(lines.join('\n')))
      << "\n```\n";

    const QString prev = DiagLog::instance()->previousRun();
    if (!prev.isEmpty()) {
        section(o, "Log of the previous run (tail)");
        o << "```\n" << red.log(prev) << "```\n";
    }

    o.flush();
    /* The log sections are already redacted with IPv4 included; the rest
     * gets the version-number-safe pass. Running text() twice over the log
     * parts is harmless. */
    return red.text(raw);
}

QString DiagReport::save(const QString &text)
{
    const QString dir = QDir::homePath() + QStringLiteral("/Documents");
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/pipecam-report-")
            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
            + QStringLiteral(".txt");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QString();
    f.write(text.toUtf8());
    return path;
}
