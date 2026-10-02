/*
 * usbdump.h — what an "lsusb -v" would say about the camera, without root and
 * without lsusb (Sailfish does not ship usbutils).
 *
 * libusb reads the descriptors from sysfs, so the full device / configuration
 * / interface / endpoint tree is available to an unprivileged process. The
 * kernel side — which driver holds which interface, how the node is
 * permissioned, the Type-C port's role — is plain sysfs as well.
 *
 * Run by the report on the GUI thread with its own libusb context, so it works
 * whether or not the camera worker is running, and in particular when the
 * worker is the thing that fails.
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

/* Every known camera on the bus, in full, plus a one-line-per-device list of
 * everything else. `claimTest` additionally claims and releases both
 * interfaces — only call it with the camera worker stopped, or it reports the
 * app's own claim as "busy". */
Result dump(bool claimTest);

/* Type-C port roles, the host controllers, and the udev rule files that decide
 * the node's permissions. */
QString hostSide();

/* Processes of this user that hold one of `nodes` open. Without root, other
 * users' processes are invisible — the root helper covers those. */
QString holders(const QStringList &nodes);

} // namespace usbdump

#endif // USBDUMP_H
