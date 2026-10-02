/*
 * diaglog.cpp — see diaglog.h.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "diaglog.h"

#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

/* Enough for a few minutes of verbose trace including a failed handshake loop
 * retrying once a second, small enough to show on one page. */
static const int MAX_LINES = 3000;
/* The file is for "what happened before the crash", not an archive. */
static const qint64 MAX_FILE_BYTES = 2 * 1024 * 1024;

QAtomicInt DiagLog::s_verbose(0);
static QtMessageHandler s_previousHandler = 0;

static QString cacheDir()
{
    return QDir::homePath() + QStringLiteral("/.cache/harbour-pipecam");
}

DiagLog *DiagLog::instance()
{
    static DiagLog *inst = new DiagLog();
    return inst;
}

DiagLog::DiagLog(QObject *parent)
    : QObject(parent)
    , m_fileBytes(0)
{
    /* Keep the last run's file before this run can overwrite it. It exists
     * only if verbose was on last time, which is exactly when it is wanted. */
    QDir().mkpath(cacheDir());
    const QString cur = cacheDir() + QStringLiteral("/pipecam.log");
    const QString prev = cacheDir() + QStringLiteral("/pipecam.prev.log");
    if (QFile::exists(cur)) {
        QFile::remove(prev);
        QFile::rename(cur, prev);
    }
    QFile p(prev);
    if (p.open(QIODevice::ReadOnly)) {
        /* Only the tail matters: the end of a run is where it went wrong. */
        if (p.size() > 256 * 1024)
            p.seek(p.size() - 256 * 1024);
        m_previousRun = QString::fromUtf8(p.readAll());
    }
}

QString DiagLog::logFile() const
{
    return cacheDir() + QStringLiteral("/pipecam.log");
}

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    const char *level = "debug";
    switch (type) {
    case QtDebugMsg:    level = "debug"; break;
    case QtInfoMsg:     level = "info";  break;
    case QtWarningMsg:  level = "warn";  break;
    case QtCriticalMsg: level = "crit";  break;
    case QtFatalMsg:    level = "fatal"; break;
    }
    DiagLog::instance()->append(QString::fromLatin1(level) + QStringLiteral(": ") + msg);
    if (s_previousHandler)
        s_previousHandler(type, ctx, msg);
}

void DiagLog::install()
{
    instance();
    s_previousHandler = qInstallMessageHandler(messageHandler);
}

bool DiagLog::isVerbose()
{
    return s_verbose.loadAcquire() != 0;
}

void DiagLog::setVerbose(bool on)
{
    if (isVerbose() == on)
        return;
    s_verbose.storeRelease(on ? 1 : 0);
    {
        QMutexLocker lock(&m_lock);
        if (on)
            openFile();
        else
            m_file.close();
    }
    append(on ? QStringLiteral("diag: verbose log on") : QStringLiteral("diag: verbose log off"));
    emit verboseChanged();
}

/* Called with m_lock held. */
void DiagLog::openFile()
{
    if (m_file.isOpen())
        return;
    m_file.setFileName(logFile());
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_fileBytes = m_file.size();
        /* What was logged before verbose was switched on belongs in the file
         * too: it is usually the start of the story. */
        for (const QString &l : m_lines) {
            const QByteArray b = l.toUtf8() + '\n';
            m_file.write(b);
            m_fileBytes += b.size();
        }
        m_file.flush();
    }
}

void DiagLog::append(const QString &line)
{
    const QString stamped = QDateTime::currentDateTime()
            .toString(QStringLiteral("HH:mm:ss.zzz ")) + line;
    QMutexLocker lock(&m_lock);
    m_lines.append(stamped);
    while (m_lines.size() > MAX_LINES)
        m_lines.removeFirst();
    if (m_file.isOpen() && m_fileBytes < MAX_FILE_BYTES) {
        const QByteArray b = stamped.toUtf8() + '\n';
        m_file.write(b);
        m_fileBytes += b.size();
        if (m_fileBytes >= MAX_FILE_BYTES)
            m_file.write("diag: log file full, further lines kept in memory only\n");
        /* Flushed per line on purpose: the file exists for the run that does
         * not end cleanly. */
        m_file.flush();
    }
}

QStringList DiagLog::lines() const
{
    QMutexLocker lock(&m_lock);
    return m_lines;
}
