/*
 * Plain-text diagnostic report: App, System, USB, Root (helper only), Log.
 * Everything passes through the Redactor before it is returned.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef DIAGREPORT_H
#define DIAGREPORT_H

#include <QObject>
#include <QVariantMap>

class DiagReport : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ appVersion CONSTANT)
public:
    explicit DiagReport(QObject *parent = 0);

    /* appState: flat key/value pairs from QML.
     * claimTest: only with the camera worker stopped. */
    Q_INVOKABLE QString build(const QVariantMap &appState, bool claimTest, bool useRoot);

    /* writes to ~/Documents; returns path or "" */
    Q_INVOKABLE QString save(const QString &text);

    static QString appVersion();
};

#endif // DIAGREPORT_H
