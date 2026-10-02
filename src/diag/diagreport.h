/*
 * diagreport.h — the diagnostic report behind About → "Create diagnostic
 * report".
 *
 * One plain-text document, built on demand, meant to be pasted into a GitHub
 * issue or an OpenRepos comment as it stands:
 *
 *   App        version, Qt, libusb, GStreamer and the two plugins recording
 *              needs, the app's settings and the camera's current state
 *   System     Sailfish OS release, phone model, kernel release, whether
 *              Android App Support is running (it can grab USB devices)
 *   USB        every device on the bus by ID, the camera's full descriptor
 *              tree ("lsusb -v" without lsusb), interface ownership, device
 *              node permissions, Type-C role, host controller, udev rules,
 *              an optional claim test, and which processes hold the camera
 *   Root       (only with the helper) kernel USB table, holders of any user,
 *              kernel log and journal excerpts, real lsusb if installed
 *   Log        this run's log, and the previous run's if verbose was on
 *
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

    /* appState: what only QML knows — camera status and the settings group,
     * as flat key/value pairs. claimTest: the camera worker is stopped, so
     * claiming the interfaces tests someone else's hold, not ours. */
    Q_INVOKABLE QString build(const QVariantMap &appState, bool claimTest, bool useRoot);

    /* Write the report to ~/Documents and return the path, or "" on failure. */
    Q_INVOKABLE QString save(const QString &text);

    static QString appVersion();
};

#endif // DIAGREPORT_H
