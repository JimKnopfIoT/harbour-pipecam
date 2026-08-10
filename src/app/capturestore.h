/*
 * capturestore.h — where captures go, and the model that lists them.
 *
 * STORAGE LAYOUT
 * --------------
 *   ~/Pictures/pipecam/PipeCam_YYYYMMDD-HHMMSS.jpg
 *   ~/Pictures/pipecam/PipeCam_YYYYMMDD-HHMMSS.mp4
 *
 * We deliberately write into the user's ordinary Pictures folder rather than
 * somewhere private to the app. Captures from a pipe inspection are evidence
 * the user needs to get off the phone — into the gallery, an email, a report.
 * Burying them in a per-application data directory would make that harder for
 * no benefit. The pipecam/ subfolder keeps them out of the camera roll, and
 * photos and videos share it so one job stays in one place.
 *
 * SNAPSHOTS ARE A FILE COPY, NOT AN ENCODE
 * ----------------------------------------
 * The camera streams MJPEG, so the frame on screen already *is* a JPEG file.
 * saveSnapshot() writes those exact bytes: no decode, no re-encode, bit-for-bit
 * what the sensor produced.
 *
 * The exception is when the pixels genuinely differ from what the camera sent —
 * a burnt-in timestamp, software brightness, or capture rotation. Then the
 * frame has to be rendered and re-encoded, because otherwise the file would not
 * match what was on screen when the shutter was pressed.
 *
 * FILENAMES
 * ---------
 * Second-resolution timestamps, so files sort chronologically by name in any
 * file manager. If two captures land in the same second (easy — the hardware
 * button bounces, and burst-tapping the shutter is natural) a -2, -3 … suffix
 * is appended rather than overwriting.
 *
 * ANONYMITY
 * ---------
 * Nothing identifying is written: no EXIF is added (the camera's JPEGs carry
 * none), no GPS, no device name. See CLAUDE.md rule 2.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef CAPTURESTORE_H
#define CAPTURESTORE_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

class UppCamera;

class CaptureStore : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString pictureDir READ pictureDir CONSTANT)
    Q_PROPERTY(QString videoDir READ videoDir CONSTANT)
    Q_PROPERTY(QString lastCapturePath READ lastCapturePath NOTIFY lastCaptureChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        FileNameRole,
        IsVideoRole,
        TimestampRole,
        SizeBytesRole,
        SizeTextRole,
        /* file:// URL, because QML's Image/VideoOutput want a URL, not a path */
        UrlRole
    };

    enum CaptureType { Photo, Video };
    Q_ENUMS(CaptureType)

    explicit CaptureStore(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int count() const { return m_items.count(); }
    QString pictureDir() const { return m_pictureDir; }
    QString videoDir() const { return m_videoDir; }
    QString lastCapturePath() const { return m_lastCapturePath; }

    /* Write the camera's current frame to a new timestamped file. Returns the
     * path, or an empty string on failure (with error() emitted).
     *
     * Takes the camera rather than the bytes ON PURPOSE. QML converts a
     * QByteArray argument to a JavaScript string, which mangles every byte
     * above 0x7F — it would silently corrupt every JPEG. Passing the QObject
     * keeps the pixels entirely inside C++.
     *
     * `stampText` empty (the normal case) writes the camera's own JPEG bytes
     * byte for byte — no decode, no re-encode, no generational loss.
     *
     * `stampText` non-empty burns that text into the bottom-right corner, which
     * unavoidably means re-encoding: you cannot draw on a JPEG without decoding
     * it first. We re-encode at quality 95, which is visually lossless at this
     * resolution, but it is a real difference and the reason the timestamp is
     * opt-in rather than always on. */
    Q_INVOKABLE QString saveSnapshot(UppCamera *camera,
                                     const QString &stampText = QString(),
                                     qreal rotation = 0.0);

    /* Reserve a filename for a video about to be recorded. The file itself is
     * created by the recorder; call registerCapture() once it is complete. */
    Q_INVOKABLE QString newVideoPath();

    /* Add an already-written file to the model (used for finished recordings). */
    Q_INVOKABLE void registerCapture(const QString &path);

    Q_INVOKABLE bool remove(int index);

    /* Rename a capture, keeping its extension. `newBaseName` is the filename
     * without the extension, exactly as the user typed it. Returns false and
     * emits error() if the name is unusable or the target already exists.
     *
     * The extension is not the user's to change: it is what the file actually
     * contains, and letting a .mp4 be renamed to .jpg would break the gallery
     * and every other app that opens it. */
    Q_INVOKABLE bool rename(int index, const QString &newBaseName);

    /* Filename without its extension — for prefilling the rename field. */
    Q_INVOKABLE QString baseName(int index) const;

    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void lastCaptureChanged();
    void error(const QString &message);
    /* A new capture landed — QML uses this to flash the shutter feedback. */
    void captured(const QString &path, bool isVideo);

private:
    struct Item {
        QString path;
        QString fileName;
        bool isVideo;
        QDateTime timestamp;
        qint64 sizeBytes;
    };

    /* Build a collision-free path in `dir` with the given extension. */
    QString makePath(const QString &dir, const QString &extension) const;
    bool ensureDir(const QString &dir);
    static QString humanSize(qint64 bytes);
    void insertItem(const QString &path, bool announce);

    QString m_pictureDir;
    QString m_videoDir;
    QString m_lastCapturePath;
    /* Newest first — that is the order a gallery should show. */
    QList<Item> m_items;
};

#endif // CAPTURESTORE_H
