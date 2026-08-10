/*
 * capturestore.cpp — see capturestore.h.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#include "capturestore.h"
#include "frameoverlay.h"
#include "uppcamera.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

static const char *SUBDIR       = "pipecam";
static const char *FILE_PREFIX  = "PipeCam_";
static const char *STAMP_FORMAT = "yyyyMMdd-HHmmss";

CaptureStore::CaptureStore(QObject *parent)
    : QAbstractListModel(parent)
{
    /* Photos AND videos both live in ~/Pictures/pipecam.
     *
     * Putting the recordings under Videos/ would be the conventional choice,
     * but it splits one inspection across two folders — and everything the app
     * produces belongs to the same job, gets reviewed together and gets copied
     * off the phone together. One folder is what makes that a single drag. */
    const QString base =
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
            + QLatin1Char('/') + QLatin1String(SUBDIR);
    m_pictureDir = base;
    m_videoDir = base;
    refresh();
}

int CaptureStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.count();
}

QHash<int, QByteArray> CaptureStore::roleNames() const
{
    QHash<int, QByteArray> r;
    r[PathRole]      = "path";
    r[FileNameRole]  = "fileName";
    r[IsVideoRole]   = "isVideo";
    r[TimestampRole] = "timestamp";
    r[SizeBytesRole] = "sizeBytes";
    r[SizeTextRole]  = "sizeText";
    r[UrlRole]       = "url";
    return r;
}

QVariant CaptureStore::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.count())
        return QVariant();

    const Item &it = m_items.at(index.row());
    switch (role) {
    case PathRole:      return it.path;
    case FileNameRole:  return it.fileName;
    case IsVideoRole:   return it.isVideo;
    case TimestampRole: return it.timestamp;
    case SizeBytesRole: return it.sizeBytes;
    case SizeTextRole:  return humanSize(it.sizeBytes);
    case UrlRole:       return QUrl::fromLocalFile(it.path).toString();
    }
    return QVariant();
}

QString CaptureStore::humanSize(qint64 bytes)
{
    if (bytes < 1024)
        return tr("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return tr("%1 kB").arg(qreal(bytes) / 1024.0, 0, 'f', 0);
    return tr("%1 MB").arg(qreal(bytes) / (1024.0 * 1024.0), 0, 'f', 1);
}

bool CaptureStore::ensureDir(const QString &dir)
{
    QDir d(dir);
    if (d.exists())
        return true;
    if (QDir().mkpath(dir))
        return true;
    emit error(tr("Cannot create folder %1").arg(dir));
    return false;
}

QString CaptureStore::makePath(const QString &dir, const QString &extension) const
{
    const QString stamp = QDateTime::currentDateTime().toString(QLatin1String(STAMP_FORMAT));
    const QString base = dir + QLatin1Char('/') + QLatin1String(FILE_PREFIX) + stamp;

    QString candidate = base + QLatin1Char('.') + extension;
    /* Second-resolution stamps collide easily (double-tap, button bounce), so
     * disambiguate rather than silently overwriting a capture. */
    int n = 2;
    while (QFile::exists(candidate) && n < 1000) {
        candidate = base + QLatin1Char('-') + QString::number(n) + QLatin1Char('.') + extension;
        ++n;
    }
    return candidate;
}

QString CaptureStore::saveSnapshot(UppCamera *camera, const QString &stampText,
                                   qreal rotation)
{
    if (!camera) {
        emit error(tr("No camera."));
        return QString();
    }

    const QByteArray jpeg = camera->currentJpeg();
    if (jpeg.isEmpty()) {
        emit error(tr("No frame to save yet."));
        return QString();
    }
    if (!ensureDir(m_pictureDir))
        return QString();

    const QString path = makePath(m_pictureDir, QLatin1String("jpg"));

    /* Two things force a re-encode: a burnt-in timestamp, and software
     * brightness. Both change the pixels, and the camera's own JPEG has
     * neither — saving the raw bytes with gain turned up would hand back a dark
     * picture that looks nothing like what was on screen when the shutter was
     * pressed. WYSIWYG wins over losslessness here; when neither is in use we
     * still take the byte-exact path below. */
    const bool needsRender = !stampText.isEmpty()
                          || !qFuzzyIsNull(rotation)
                          || camera->gain() > 1.001;

    if (!needsRender) {
        /* Fast path: the camera's bytes, unmodified. */
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            emit error(tr("Cannot write %1: %2").arg(path, f.errorString()));
            return QString();
        }
        const qint64 written = f.write(jpeg);
        f.close();
        if (written != jpeg.size()) {
            emit error(tr("Only wrote %1 of %2 bytes to %3")
                       .arg(written).arg(jpeg.size()).arg(path));
            QFile::remove(path);
            return QString();
        }
    } else {
        /* Rendered path: start from the frame the worker already decoded (and
         * already brightened) for display — no second JPEG decode — then
         * re-encode once. */
        QImage image = camera->currentImage();
        if (image.isNull())
            image = QImage::fromData(jpeg, "JPEG");
        if (image.isNull()) {
            emit error(tr("Could not decode the frame to add the timestamp."));
            return QString();
        }
        /* fromData() can hand back a shared/read-only image; make sure we own
         * the pixels before painting on them. */
        image = image.convertToFormat(QImage::Format_RGB32);
        /* Rotate first, stamp second, so the timestamp stays level and readable
         * however far the picture has been turned. */
        image = overlay::rotateFit(image, rotation);
        overlay::drawTimestamp(&image, stampText);

        if (!image.save(path, "JPEG", 95)) {
            emit error(tr("Cannot write %1").arg(path));
            return QString();
        }
    }

    insertItem(path, true);
    return path;
}

QString CaptureStore::newVideoPath()
{
    if (!ensureDir(m_videoDir))
        return QString();
    return makePath(m_videoDir, QLatin1String("mp4"));
}

void CaptureStore::registerCapture(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path))
        return;
    /* A finished recording may already be listed if refresh() ran meanwhile. */
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i).path == path) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();
            break;
        }
    }
    insertItem(path, true);
}

void CaptureStore::insertItem(const QString &path, bool announce)
{
    const QFileInfo fi(path);
    Item it;
    it.path = fi.absoluteFilePath();
    it.fileName = fi.fileName();
    it.isVideo = fi.suffix().compare(QLatin1String("mp4"), Qt::CaseInsensitive) == 0;
    it.timestamp = fi.lastModified();
    it.sizeBytes = fi.size();

    /* Newest first. */
    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(it);
    endInsertRows();
    emit countChanged();

    m_lastCapturePath = it.path;
    emit lastCaptureChanged();
    if (announce)
        emit captured(it.path, it.isVideo);
}

bool CaptureStore::remove(int index)
{
    if (index < 0 || index >= m_items.count())
        return false;

    const QString path = m_items.at(index).path;
    if (QFile::exists(path) && !QFile::remove(path)) {
        emit error(tr("Cannot delete %1").arg(QFileInfo(path).fileName()));
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit countChanged();

    if (m_lastCapturePath == path) {
        m_lastCapturePath = m_items.isEmpty() ? QString() : m_items.first().path;
        emit lastCaptureChanged();
    }
    return true;
}

QString CaptureStore::baseName(int index) const
{
    if (index < 0 || index >= m_items.count())
        return QString();
    return QFileInfo(m_items.at(index).path).completeBaseName();
}

bool CaptureStore::rename(int index, const QString &newBaseName)
{
    if (index < 0 || index >= m_items.count())
        return false;

    const QString wanted = newBaseName.trimmed();
    if (wanted.isEmpty()) {
        emit error(tr("The name cannot be empty."));
        return false;
    }
    /* Reject anything that would move the file somewhere else or produce a
     * hidden file. Renaming is not a file manager. */
    if (wanted.contains(QLatin1Char('/')) || wanted.startsWith(QLatin1Char('.'))) {
        emit error(tr("A name cannot contain “/” or start with a dot."));
        return false;
    }

    const QFileInfo fi(m_items.at(index).path);
    const QString target = fi.absolutePath() + QLatin1Char('/')
                         + wanted + QLatin1Char('.') + fi.suffix();

    if (target == fi.absoluteFilePath())
        return true;                       /* nothing to do */

    if (QFile::exists(target)) {
        emit error(tr("“%1” already exists.").arg(QFileInfo(target).fileName()));
        return false;
    }
    if (!QFile::rename(fi.absoluteFilePath(), target)) {
        emit error(tr("Could not rename “%1”.").arg(fi.fileName()));
        return false;
    }

    Item &it = m_items[index];
    it.path = target;
    it.fileName = QFileInfo(target).fileName();

    const QModelIndex mi = this->index(index, 0);
    /* Only the name-derived roles changed; size, type and timestamp are the
     * same file. */
    QVector<int> roles;
    roles << PathRole << FileNameRole << UrlRole;
    emit dataChanged(mi, mi, roles);

    if (m_lastCapturePath == fi.absoluteFilePath()) {
        m_lastCapturePath = target;
        emit lastCaptureChanged();
    }
    return true;
}

void CaptureStore::refresh()
{
    beginResetModel();
    m_items.clear();

    QStringList picFilter;
    picFilter << QLatin1String("*.jpg") << QLatin1String("*.jpeg");
    QStringList vidFilter;
    vidFilter << QLatin1String("*.mp4");

    QList<QFileInfo> found;
    found += QDir(m_pictureDir).entryInfoList(picFilter, QDir::Files);
    /* Photos and videos currently share one folder. Scanning it twice would
     * list every file twice, so only walk the video folder when it really is a
     * different directory — this keeps working if they are ever split again. */
    if (m_videoDir == m_pictureDir)
        found += QDir(m_pictureDir).entryInfoList(vidFilter, QDir::Files);
    else
        found += QDir(m_videoDir).entryInfoList(vidFilter, QDir::Files);

    /* Sort newest first across both folders. Insertion sort over a handful of
     * hundreds of files is not worth optimising, and it keeps the comparison
     * logic in one obvious place. */
    for (int i = 0; i < found.count(); ++i) {
        const QFileInfo &fi = found.at(i);
        Item it;
        it.path = fi.absoluteFilePath();
        it.fileName = fi.fileName();
        it.isVideo = fi.suffix().compare(QLatin1String("mp4"), Qt::CaseInsensitive) == 0;
        it.timestamp = fi.lastModified();
        it.sizeBytes = fi.size();

        int pos = 0;
        while (pos < m_items.count() && m_items.at(pos).timestamp > it.timestamp)
            ++pos;
        m_items.insert(pos, it);
    }

    endResetModel();
    emit countChanged();

    const QString newest = m_items.isEmpty() ? QString() : m_items.first().path;
    if (newest != m_lastCapturePath) {
        m_lastCapturePath = newest;
        emit lastCaptureChanged();
    }
}
