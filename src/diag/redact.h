/*
 * Removes: serials, host name, user name, home dir, MAC, e-mail,
 * 15-20 digit runs, USB SerialNumber/iSerial values; IPv4 in log() only
 * (four-part version numbers look like IPv4).
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef REDACT_H
#define REDACT_H

#include <QString>
#include <QStringList>

class Redactor
{
public:
    Redactor();

    /* exact match; values < 3 chars are ignored */
    void addSecret(const QString &value, const QString &replacement);

    /* everything but IPv4 */
    QString text(const QString &in) const;
    /* text() plus IPv4; for log lines */
    QString log(const QString &in) const;

private:
    struct Secret { QString value; QString replacement; };
    QList<Secret> m_secrets;
    /* host name: whole-word match at any length */
    QString m_host;
};

#endif // REDACT_H
