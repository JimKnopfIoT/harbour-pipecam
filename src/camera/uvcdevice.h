/*
 * USB Video Class cameras via uvcvideo (/dev/videoN). Fallback when no
 * USeePlus device is on the bus. Receive only: no UVC controls are set.
 *   scan():  sysfs, USB interfaces of class 0e/01 -> video4linux node.
 *   Stream:  V4L2 mmap capture, YUYV preferred, MJPEG second.
 *   Button:  uvcvideo input device (KEY_CAMERA), if the kernel created one;
 *            it does only when bTriggerSupport = bTriggerUsage = 1.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef UVCDEVICE_H
#define UVCDEVICE_H

#include "usertext.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

namespace uvc {

/* USB device with a video control interface. */
struct UsbDevice {
    QString sysName;    /* e.g. "1-1" */
    QString sysDir;     /* canonical /sys/devices/... path */
    int vid;
    int pid;
};

QList<UsbDevice> usbDevices();

/* /dev/videoN of the device, capture and metadata nodes. */
QStringList videoNodes(const UsbDevice &d);
/* /dev/input/eventN of the device. */
QStringList inputNodes(const UsbDevice &d);
/* sysfs attribute, trimmed; "" if absent */
QString sysAttr(const QString &dir, const char *attr);

enum ScanResult {
    None,       /* no UVC device */
    NoNode,     /* device without /dev/video (no uvcvideo driver) */
    NoAccess,   /* node exists, open() fails */
    Found
};

struct Candidate {
    UsbDevice usb;
    QString node;
    QString card;
};

/* err set for NoNode / NoAccess. */
ScanResult scan(Candidate *c, UserText *err);

class Stream
{
public:
    enum Result { Frame, Timeout, Gone };

    Stream();
    ~Stream();

    bool open(const Candidate &c, UserText *err);
    void close();

    /* Waits up to timeoutMs. Button edges (true = pressed) appended to
     * buttons in arrival order. On Frame: data()/bytes() valid until requeue(). */
    Result next(int timeoutMs, QVector<bool> *buttons);
    void requeue();

    const unsigned char *data() const { return m_data; }
    int bytes() const { return m_bytes; }

    bool isMjpeg() const;
    int width() const { return m_width; }
    int height() const { return m_height; }
    int stride() const { return m_stride; }
    QString formatName() const;
    /* all offered formats and sizes, one line each */
    QString offered() const { return m_offered; }
    QString buttonNode() const { return m_buttonNode; }

    QVariantMap deviceInfo(const Candidate &c) const;

private:
    struct Buf { void *start; size_t len; };

    void readButton(QVector<bool> *buttons);

    int m_fd;
    int m_evFd;
    QString m_buttonNode;
    unsigned m_pixfmt;
    int m_width;
    int m_height;
    int m_stride;
    QString m_offered;
    Buf m_bufs[8];
    int m_nbuf;
    bool m_streaming;
    int m_dequeued;     /* index or -1 */
    const unsigned char *m_data;
    int m_bytes;
    int m_badFrames;
};

/* true if the JPEG has a DHT segment before SOS (UVC MJPEG often omits it). */
bool jpegHasDht(const unsigned char *p, int n);

} // namespace uvc

#endif // UVCDEVICE_H
