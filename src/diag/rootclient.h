/*
 * Client for roothelper. Start/stop via systemd StartUnit/StopUnit on the
 * system bus (polkit: this unit only). Requests block.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef ROOTCLIENT_H
#define ROOTCLIENT_H

#include <QLocalSocket>
#include <QObject>
#include <QTimer>

class RootClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    static RootClient *instance();
    static QString socketPath();

    bool active() const { return m_sock.state() == QLocalSocket::ConnectedState; }
    QString lastError() const { return m_lastError; }

    /* stopping also drops the connection */
    Q_INVOKABLE void setHelper(bool on);

    /* helper command letter; empty on failure */
    QByteArray request(char cmd, int timeoutMs);

signals:
    void activeChanged();
    void lastErrorChanged();

private:
    explicit RootClient(QObject *parent = 0);
    void probe();

    QLocalSocket m_sock;
    QTimer m_retry;
    bool m_wanted;
    QString m_lastError;
};

#endif // ROOTCLIENT_H
