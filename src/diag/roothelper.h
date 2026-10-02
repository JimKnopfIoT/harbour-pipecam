/*
 * roothelper.h — optional privileged half of the diagnostic report.
 *
 * Same pattern as harbour-sysmetrics: the app binary itself, started as
 * `harbour-pipecam --root-helper` by the systemd unit harbour-pipecam-helper
 * .service, which a polkit rule lets defaultuser start and stop and nothing
 * else. Never enabled at boot; it exits on its own 20 s after the last client
 * is gone.
 *
 * It answers a fixed set of read-only questions about the camera and nothing
 * more — no paths, no arguments, no writes:
 *
 *   H   which processes, of any user, hold the camera's /dev/bus/usb node
 *       (the answer to "camera is busy" when the culprit is a system service
 *       or the Android container)
 *   U   the kernel's own USB device table from debugfs, camera entries only —
 *       shows which driver has claimed which interface
 *   K   kernel log lines about USB, last 200
 *   J   journal lines about USB / PipeCam from this boot, last 300
 *   L   `lsusb -v` for the camera IDs, if usbutils happens to be installed
 *
 * The answers are raw; the app redacts them before showing them.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef ROOTHELPER_H
#define ROOTHELPER_H

int rootHelperMain(int argc, char *argv[]);

#endif // ROOTHELPER_H
