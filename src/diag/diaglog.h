/*
 * diaglog.h — the app's own log, for the diagnostic report.
 *
 * Every qDebug/qWarning the process produces (C++ and QML alike) passes
 * through here on its way to the journal, and the last few thousand lines are
 * kept in memory so the report can include them without root.
 *
 * "Verbose" adds two things, both off by default because they cost something:
 *
 *   * libusb's own debug output and the worker's step-by-step trace of the
 *     open/claim/handshake sequence — the part that explains a camera that is
 *     found but will not start;
 *   * a copy of every line in ~/.cache/harbour-pipecam/pipecam.log, so a run
 *     that ends in a crash or a hang can still be reported after a restart. The
 *     previous run's file is kept as pipecam.prev.log.
 *
 * Nothing here leaves the device. The report built from it is anonymised
 * (see redact.h) and only ever copied or saved by the user.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef DIAGLOG_H
#define DIAGLOG_H

#include <QAtomicInt>
#include <QFile>
#include <QMutex>
#include <QObject>
#include <QStringList>

class DiagLog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY verboseChanged)
    Q_PROPERTY(QString logFile READ logFile CONSTANT)

public:
    static DiagLog *instance();

    /* Route Qt's message output through the log. Call once, early in main(). */
    static void install();

    /* Cheap enough for the worker thread to ask before every trace line. */
    static bool isVerbose();

    bool verbose() const { return isVerbose(); }
    void setVerbose(bool on);

    QString logFile() const;

    /* Thread-safe. `line` has no trailing newline. */
    void append(const QString &line);

    /* Snapshot of the in-memory lines, oldest first. */
    QStringList lines() const;
    /* The previous run's file, or an empty string. Read once at startup. */
    QString previousRun() const { return m_previousRun; }

signals:
    void verboseChanged();

private:
    explicit DiagLog(QObject *parent = 0);
    void openFile();

    static QAtomicInt s_verbose;

    mutable QMutex m_lock;
    QStringList m_lines;
    QFile m_file;
    qint64 m_fileBytes;
    QString m_previousRun;
};

/* Trace line from anywhere, written only while verbose is on. Kept as a macro
 * so the string is not even built when it is off. */
#define PIPECAM_TRACE(msg) \
    do { if (DiagLog::isVerbose()) DiagLog::instance()->append(QStringLiteral("trace: ") + (msg)); } while (0)

#endif // DIAGLOG_H
