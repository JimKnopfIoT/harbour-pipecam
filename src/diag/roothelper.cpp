/*
 * Protocol: one command letter per line; answer "OK <len>\n<payload>" or "ERR\n".
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   // struct ucred
#endif
#include "roothelper.h"
#include "rootclient.h"
#include "uppprotocol.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include <dirent.h>
#include <stdio.h>
#include <sys/klog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

const uid_t DEFAULTUSER_UID = 100000;   // Sailfish's defaultuser

QByteArray readAll(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
}

bool isCameraId(const QByteArray &vid, const QByteArray &pid)
{
    for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i) {
        if (vid.toInt(0, 16) == upp::KNOWN_DEVICES[i].vid &&
            pid.toInt(0, 16) == upp::KNOWN_DEVICES[i].pid)
            return true;
    }
    return false;
}

/* from sysfs; the client never supplies a path */
QStringList cameraNodes()
{
    QStringList out;
    const QDir usb(QStringLiteral("/sys/bus/usb/devices"));
    for (const QString &d : usb.entryList(QDir::Dirs | QDir::System | QDir::NoDotAndDotDot)) {
        const QString b = usb.absoluteFilePath(d);
        if (!isCameraId(readAll(b + "/idVendor"), readAll(b + "/idProduct")))
            continue;
        out << QStringLiteral("/dev/bus/usb/%1/%2")
               .arg(readAll(b + "/busnum").toInt(), 3, 10, QChar('0'))
               .arg(readAll(b + "/devnum").toInt(), 3, 10, QChar('0'));
    }
    return out;
}

QByteArray cmdHolders()
{
    const QStringList nodes = cameraNodes();
    if (nodes.isEmpty())
        return "no camera on the bus\n";
    QByteArray out;
    DIR *proc = opendir("/proc");
    while (proc) {
        struct dirent *pe = readdir(proc);
        if (!pe)
            break;
        if (pe->d_name[0] < '0' || pe->d_name[0] > '9')
            continue;
        const QByteArray fdDir = "/proc/" + QByteArray(pe->d_name) + "/fd";
        DIR *fd = opendir(fdDir.constData());
        if (!fd)
            continue;
        while (struct dirent *fe = readdir(fd)) {
            if (fe->d_name[0] < '0' || fe->d_name[0] > '9')
                continue;
            char buf[512];
            const ssize_t n = readlink((fdDir + "/" + QByteArray(fe->d_name)).constData(), buf, sizeof(buf) - 1);
            if (n <= 0)
                continue;
            const QString target = QString::fromLocal8Bit(buf, int(n));
            if (!nodes.contains(target))
                continue;
            struct stat st;
            const uid_t uid = stat(("/proc/" + QByteArray(pe->d_name)).constData(), &st) == 0
                            ? st.st_uid : uid_t(-1);
            out += "pid " + QByteArray(pe->d_name) + " (" +
                   readAll("/proc/" + QString::fromLatin1(pe->d_name) + "/comm") +
                   ") uid " + QByteArray::number(uint(uid)) + " holds " + target.toLocal8Bit() + '\n';
        }
        closedir(fd);
    }
    if (proc)
        closedir(proc);
    if (out.isEmpty())
        out = "no process holds " + nodes.join(QStringLiteral(", ")).toLocal8Bit() + '\n';
    return out;
}

/* blank-line separated blocks; camera blocks only */
QByteArray cmdUsbTable()
{
    QFile f(QStringLiteral("/sys/kernel/debug/usb/devices"));
    if (!f.open(QIODevice::ReadOnly))
        return "debugfs usb/devices not available\n";
    QByteArray out;
    const QStringList blocks = QString::fromLatin1(f.readAll()).split(QStringLiteral("\n\n"));
    for (const QString &block : blocks) {
        for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i) {
            const QString tag = QStringLiteral("Vendor=%1 ProdID=%2")
                    .arg(upp::KNOWN_DEVICES[i].vid, 4, 16, QChar('0'))
                    .arg(upp::KNOWN_DEVICES[i].pid, 4, 16, QChar('0'));
            if (block.contains(tag)) {
                out += block.trimmed().toLatin1() + "\n\n";
                break;
            }
        }
    }
    return out.isEmpty() ? QByteArray("no camera entry in debugfs usb/devices\n") : out;
}

/* whole words; no tcpm/type-c: charger and PD drivers flood the log */
bool aboutUsb(const QByteArray &line)
{
    static const QRegularExpression re(
        QStringLiteral("\\b(usb|usbfs|usbcore|usbhid|xhci[-_a-z0-9]*|ehci[-_a-z0-9]*|dwc3|mtu3|ssusb|"
                       "typec|ucsi|2ce3|0329|pipecam|supercamera)\\b|libusb|geek szitman"),
        QRegularExpression::CaseInsensitiveOption);
    return re.match(QString::fromLocal8Bit(line)).hasMatch();
}

QByteArray keepTail(const QList<QByteArray> &lines, int max)
{
    QByteArray out;
    const int from = lines.size() > max ? lines.size() - max : 0;
    for (int i = from; i < lines.size(); ++i)
        out += lines.at(i) + '\n';
    return out;
}

QByteArray cmdKernelLog()
{
    int len = klogctl(10 /* SIZE_BUFFER */, 0, 0);
    if (len <= 0)
        len = 1 << 20;
    QByteArray buf(len + 1, 0);
    const int n = klogctl(3 /* READ_ALL */, buf.data(), len);
    if (n <= 0)
        return "kernel log not readable\n";
    buf.truncate(n);
    QList<QByteArray> keep;
    for (QByteArray l : buf.split('\n')) {
        if (l.startsWith('<')) {
            const int gt = l.indexOf('>');
            if (gt > 0)
                l = l.mid(gt + 1);
        }
        if (aboutUsb(l))
            keep << l;
    }
    return keep.isEmpty() ? QByteArray("no matching kernel log lines\n") : keepTail(keep, 200);
}

QByteArray cmdJournal()
{
    QProcess jp;
    /* monotonic: no wall-clock time in the report */
    jp.start(QStringLiteral("journalctl"),
             QStringList() << QStringLiteral("-b") << QStringLiteral("--no-pager")
                           << QStringLiteral("-n") << QStringLiteral("20000")
                           << QStringLiteral("-o") << QStringLiteral("short-monotonic"));
    if (!jp.waitForFinished(15000))
        return "journalctl not available\n";
    QList<QByteArray> keep;
    for (QByteArray l : jp.readAllStandardOutput().split('\n')) {
        if (!aboutUsb(l))
            continue;
        /* drop host column: "[ <s>] host ident[pid]: msg" */
        const int close = l.indexOf("] ");
        if (l.startsWith('[') && close > 0) {
            const int sp = l.indexOf(' ', close + 2);
            if (sp > 0)
                l = l.left(close + 2) + l.mid(sp + 1);
        }
        keep << l;
    }
    return keep.isEmpty() ? QByteArray("no matching journal lines\n") : keepTail(keep, 300);
}

QByteArray cmdLsusb()
{
    const QString bin = QFile::exists(QStringLiteral("/usr/bin/lsusb")) ? QStringLiteral("/usr/bin/lsusb")
                      : QFile::exists(QStringLiteral("/bin/lsusb"))     ? QStringLiteral("/bin/lsusb")
                      : QString();
    if (bin.isEmpty())
        return "lsusb not installed (package usbutils)\n";
    QByteArray out;
    for (int i = 0; i < upp::KNOWN_DEVICE_COUNT; ++i) {
        const QString id = QStringLiteral("%1:%2")
                .arg(upp::KNOWN_DEVICES[i].vid, 4, 16, QChar('0'))
                .arg(upp::KNOWN_DEVICES[i].pid, 4, 16, QChar('0'));
        QProcess p;
        p.start(bin, QStringList() << QStringLiteral("-v") << QStringLiteral("-d") << id);
        if (p.waitForFinished(5000))
            out += p.readAllStandardOutput();
    }
    return out.isEmpty() ? QByteArray("lsusb: no camera listed\n") : out;
}

/* root and defaultuser only; enforced here in addition to socket mode */
bool peerAllowed(QLocalSocket *sock, uid_t *who)
{
    struct ucred cr;
    socklen_t len = sizeof(cr);
    if (getsockopt(int(sock->socketDescriptor()), SOL_SOCKET, SO_PEERCRED, &cr, &len) != 0)
        return false;
    *who = cr.uid;
    return cr.uid == 0 || cr.uid == DEFAULTUSER_UID;
}

void serve(QLocalSocket *sock)
{
    QObject::connect(sock, &QLocalSocket::readyRead, sock, [sock]() {
        while (sock->canReadLine()) {
            const QByteArray cmd = sock->readLine().trimmed();
            QByteArray payload;
            bool ok = true;
            if (cmd == "H")
                payload = cmdHolders();
            else if (cmd == "U")
                payload = cmdUsbTable();
            else if (cmd == "K")
                payload = cmdKernelLog();
            else if (cmd == "J")
                payload = cmdJournal();
            else if (cmd == "L")
                payload = cmdLsusb();
            else
                ok = false;
            if (ok)
                sock->write("OK " + QByteArray::number(payload.size()) + '\n' + payload);
            else
                sock->write("ERR\n");
        }
    });
    QObject::connect(sock, &QLocalSocket::disconnected, sock, &QObject::deleteLater);
}

} // namespace

int rootHelperMain(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (getuid() != 0) {
        fprintf(stderr, "pipecam helper: must run as root\n");
        return 1;
    }
    const QString path = RootClient::socketPath();
    QLocalServer::removeServer(path);
    QLocalServer server;
    if (!server.listen(path)) {
        fprintf(stderr, "pipecam helper: cannot listen on %s\n", qPrintable(path));
        return 1;
    }
    /* 0660 root:defaultuser group; peerAllowed() checks again */
    if (chown(QFile::encodeName(path).constData(), 0, DEFAULTUSER_UID) != 0 ||
        chmod(QFile::encodeName(path).constData(), 0660) != 0) {
        fprintf(stderr, "pipecam helper: cannot set socket permissions\n");
        return 1;
    }

    static int clients = 0;
    QObject::connect(&server, &QLocalServer::newConnection, &server, [&server]() {
        while (QLocalSocket *s = server.nextPendingConnection()) {
            uid_t who = uid_t(-1);
            if (!peerAllowed(s, &who)) {
                fprintf(stderr, "pipecam helper: refusing a client of uid %u\n", uint(who));
                s->abort();
                s->deleteLater();
                continue;
            }
            ++clients;
            QObject::connect(s, &QLocalSocket::disconnected, s, []() { --clients; });
            serve(s);
        }
    });

    /* exit when idle; 20 s grace for the first client */
    QElapsedTimer up;
    up.start();
    QTimer idle;
    idle.setInterval(5000);
    QObject::connect(&idle, &QTimer::timeout, &app, [&app, &up]() {
        if (clients == 0 && up.elapsed() > 20000)
            app.quit();
    });
    idle.start();
    printf("pipecam helper: listening on %s\n", qPrintable(path));
    fflush(stdout);
    const int rc = app.exec();
    QLocalServer::removeServer(path);
    return rc;
}
