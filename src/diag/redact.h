/*
 * redact.h — make a diagnostic report safe to post in public.
 *
 * The report is meant to be pasted into a GitHub issue or an OpenRepos
 * comment as it stands, so everything that identifies a person, a phone or a
 * network is removed before the user ever sees the text:
 *
 *   exact values   the camera's serial number, the phone's hostname, the
 *                  user name and home directory
 *   patterns       MAC addresses, e-mail addresses, IMEI-length digit runs,
 *                  "SerialNumber:"/"iSerial" values in USB dumps, and — in log
 *                  excerpts only — IPv4 addresses (a four-part version
 *                  number looks exactly like one, so the system section, which
 *                  is all version numbers, is left alone)
 *
 * What stays is what a bug needs: models, versions, USB IDs, descriptor
 * layout, error codes, and timings.
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

    /* Add an exact value to remove everywhere, e.g. a serial read from the
     * device. Values shorter than three characters are ignored — removing
     * "1" from a report would destroy it. */
    void addSecret(const QString &value, const QString &replacement);

    /* Everything but IPv4. */
    QString text(const QString &in) const;
    /* text() plus IPv4 — for journal and kernel log lines. */
    QString log(const QString &in) const;

private:
    struct Secret { QString value; QString replacement; };
    QList<Secret> m_secrets;
    /* The host name is matched as a whole word, at any length: a phone may be
     * given a two-letter name, far too short for a plain substring replace. */
    QString m_host;
};

#endif // REDACT_H
