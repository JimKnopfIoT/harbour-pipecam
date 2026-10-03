/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "rootclient.h"

#include <QDBusConnection>
#include <QDBusMessage>

static const char *const UNIT = "harbour-pipecam-helper.service";

RootClient *RootClient::instance()
{
    static RootClient *inst = new RootClient();
    return inst;
}

QString RootClient::socketPath()
{
    /* /run, not /tmp: only root can create it, no spoofed socket */
    return QStringLiteral("/run/harbour-pipecam-helper.sock");
}

RootClient::RootClient(QObject *parent)
    : QObject(parent)
    , m_wanted(false)
{
    connect(&m_sock, &QLocalSocket::stateChanged, this, &RootClient::activeChanged);
    m_retry.setInterval(1000);
    connect(&m_retry, &QTimer::timeout, this, [this]() {
        if (!m_wanted) {
            m_retry.stop();
            return;
        }
        if (m_sock.state() == QLocalSocket::UnconnectedState)
            probe();
    });
}

void RootClient::setHelper(bool on)
{
    m_wanted = on;
    if (!on) {
        m_sock.abort();
        m_retry.stop();
    }
    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.systemd1"),
        QStringLiteral("/org/freedesktop/systemd1"),
        QStringLiteral("org.freedesktop.systemd1.Manager"),
        on ? QStringLiteral("StartUnit") : QStringLiteral("StopUnit"));
    call << QString::fromLatin1(UNIT) << QStringLiteral("replace");
    const QDBusMessage reply = QDBusConnection::systemBus().call(call);
    const QString err = reply.type() == QDBusMessage::ErrorMessage ? reply.errorMessage() : QString();
    if (!err.isEmpty())
        qWarning("pipecam: %s helper failed: %s", on ? "start" : "stop", qPrintable(err));
    if (err != m_lastError) {
        m_lastError = err;
        emit lastErrorChanged();
    }
    if (on && err.isEmpty())
        m_retry.start();
}

void RootClient::probe()
{
    if (m_sock.state() == QLocalSocket::ConnectedState)
        return;
    m_sock.abort();
    m_sock.connectToServer(socketPath());
    if (m_sock.waitForConnected(300))
        m_retry.stop();
}

QByteArray RootClient::request(char cmd, int timeoutMs)
{
    if (m_sock.state() != QLocalSocket::ConnectedState)
        return QByteArray();
    m_sock.write(QByteArray(1, cmd) + '\n');
    if (!m_sock.waitForBytesWritten(500))
        return QByteArray();
    QByteArray header;
    while (!header.contains('\n')) {
        if (!m_sock.waitForReadyRead(timeoutMs)) {
            m_sock.abort();
            return QByteArray();
        }
        header += m_sock.readAll();
    }
    const int nl = header.indexOf('\n');
    QByteArray payload = header.mid(nl + 1);
    header.truncate(nl);
    if (!header.startsWith("OK "))
        return QByteArray();
    const int len = header.mid(3).toInt();
    while (payload.size() < len) {
        if (!m_sock.waitForReadyRead(2000)) {
            m_sock.abort();
            return QByteArray();
        }
        payload += m_sock.readAll();
    }
    payload.truncate(len);
    return payload;
}
