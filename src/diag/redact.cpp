/*
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "redact.h"

#include <QDir>
#include <QRegularExpression>

#include <unistd.h>

Redactor::Redactor()
{
    char host[256] = { 0 };
    if (::gethostname(host, sizeof(host) - 1) == 0)
        m_host = QString::fromLocal8Bit(host).trimmed();

    /* home before user name, so the home path becomes "~" */
    addSecret(QDir::homePath(), QStringLiteral("~"));
    const QString user = QDir::home().dirName();
    /* defaultuser is not identifying */
    if (user != QLatin1String("defaultuser"))
        addSecret(user, QStringLiteral("<user>"));
}

void Redactor::addSecret(const QString &value, const QString &replacement)
{
    const QString v = value.trimmed();
    if (v.size() < 3)
        return;
    for (const Secret &s : m_secrets)
        if (s.value == v)
            return;
    Secret s = { v, replacement };
    /* longest first */
    int i = 0;
    while (i < m_secrets.size() && m_secrets.at(i).value.size() >= v.size())
        ++i;
    m_secrets.insert(i, s);
}

QString Redactor::text(const QString &in) const
{
    QString out = in;
    for (const Secret &s : m_secrets)
        out.replace(s.value, s.replacement, Qt::CaseSensitive);

    if (!m_host.isEmpty() && m_host != QLatin1String("localhost")) {
        const QRegularExpression host(QStringLiteral("\\b%1\\b")
                                      .arg(QRegularExpression::escape(m_host)));
        out.replace(host, QStringLiteral("<host>"));
    }

    static const QRegularExpression mac(
        QStringLiteral("\\b(?:[0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}\\b"));
    out.replace(mac, QStringLiteral("xx:xx:xx:xx:xx:xx"));

    static const QRegularExpression mail(
        QStringLiteral("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}"));
    out.replace(mail, QStringLiteral("<email>"));

    /* IMEI/IMSI 15, ICCID 19-20 digits */
    static const QRegularExpression longNum(QStringLiteral("\\b\\d{15,20}\\b"));
    out.replace(longNum, QStringLiteral("<number>"));

    /* serials of any other USB device */
    static const QRegularExpression usbSerial(
        QStringLiteral("(SerialNumber[:=]\\s*|iSerial\\s+\\d+\\s+)\\S.*$"),
        QRegularExpression::MultilineOption);
    out.replace(usbSerial, QStringLiteral("\\1<serial>"));
    return out;
}

QString Redactor::log(const QString &in) const
{
    QString out = text(in);
    static const QRegularExpression ipv4(
        QStringLiteral("\\b(?:\\d{1,3}\\.){3}\\d{1,3}\\b"));
    out.replace(ipv4, QStringLiteral("x.x.x.x"));
    return out;
}
