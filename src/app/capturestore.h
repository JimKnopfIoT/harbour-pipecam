/*
 * Captures: ~/Pictures/pipecam/PipeCam_yyyyMMdd-HHmmss[-N].{jpg,mp4}
 * No EXIF, GPS or device name is written.
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
        /* file:// URL */
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

    /* Returns path, or empty with error() emitted.
     * Takes the camera, not a QByteArray: QML would convert bytes to a JS
     * string and corrupt bytes > 0x7F.
     * No stamp/rotation/gain: camera JPEG bytes as-is; else re-encode, q95. */
    Q_INVOKABLE QString saveSnapshot(UppCamera *camera,
                                     const QString &stampText = QString(),
                                     qreal rotation = 0.0);

    /* File is created by the recorder; call registerCapture() when done. */
    Q_INVOKABLE QString newVideoPath();

    Q_INVOKABLE void registerCapture(const QString &path);

    Q_INVOKABLE bool remove(int index);

    /* `newBaseName` without extension; extension is kept. */
    Q_INVOKABLE bool rename(int index, const QString &newBaseName);

    Q_INVOKABLE QString baseName(int index) const;

    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void lastCaptureChanged();
    void error(const QString &message);
    void captured(const QString &path, bool isVideo);

private:
    struct Item {
        QString path;
        QString fileName;
        bool isVideo;
        QDateTime timestamp;
        qint64 sizeBytes;
    };

    QString makePath(const QString &dir, const QString &extension) const;
    bool ensureDir(const QString &dir);
    static QString humanSize(qint64 bytes);
    void insertItem(const QString &path, bool announce);

    QString m_pictureDir;
    QString m_videoDir;
    QString m_lastCapturePath;
    /* newest first */
    QList<Item> m_items;
};

#endif // CAPTURESTORE_H
