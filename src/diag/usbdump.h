/*
 * Descriptor tree and sysfs state without root or lsusb.
 * GUI thread, own libusb context; independent of the camera worker.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef USBDUMP_H
#define USBDUMP_H

#include <QString>
#include <QStringList>

namespace usbdump {

struct Result {
    QString text;          /* human-readable, not yet redacted        */
    QStringList serials;   /* string descriptors to redact             */
    QStringList nodes;     /* /dev/bus/usb/BBB/DDD of every camera     */
};

/* claimTest: only with the camera worker stopped */
Result dump(bool claimTest);

/* Type-C roles, host controllers, udev rule files */
QString hostSide();

/* own user's processes only */
QString holders(const QStringList &nodes);

} // namespace usbdump

#endif // USBDUMP_H
