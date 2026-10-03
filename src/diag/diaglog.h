/*
 * In-memory ring of all Qt messages (C++ and QML), readable without root.
 * Verbose: libusb debug + worker trace, mirrored to
 * ~/.cache/harbour-pipecam/pipecam.log (previous run: pipecam.prev.log).
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

    /* call once, early in main() */
    static void install();

    /* lock-free, any thread */
    static bool isVerbose();

    bool verbose() const { return isVerbose(); }
    void setVerbose(bool on);

    QString logFile() const;

    /* thread-safe; `line` without trailing newline */
    void append(const QString &line);

    /* oldest first */
    QStringList lines() const;
    /* read once at startup */
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

/* macro: string is not built while verbose is off */
#define PIPECAM_TRACE(msg) \
    do { if (DiagLog::isVerbose()) DiagLog::instance()->append(QStringLiteral("trace: ") + (msg)); } while (0)

#endif // DIAGLOG_H
