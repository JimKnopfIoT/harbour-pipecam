/*
 * rootclient.h — the app's side of the optional root helper (roothelper.h).
 *
 * Starting the helper goes through systemd's StartUnit on the system bus; a
 * polkit rule allows exactly that one unit for defaultuser. Requests are one
 * blocking round trip each — they are only made while a report is being built
 * and the page shows a busy indicator meanwhile.
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

    /* Start or stop the helper unit. Stopping also drops the connection. */
    Q_INVOKABLE void setHelper(bool on);

    /* One of the helper's command letters; empty on any failure. */
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
