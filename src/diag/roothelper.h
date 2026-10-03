/*
 * Root helper: `harbour-pipecam --root-helper`, run by systemd unit
 * harbour-pipecam-helper.service; polkit lets defaultuser start/stop it only.
 * Never enabled at boot; exits 20 s after the last client.
 * Fixed read-only commands; no arguments, paths or writes:
 *   H  holders of the camera's /dev/bus/usb node, any user
 *   U  debugfs USB device table, camera entries only
 *   K  kernel log USB lines, last 200
 *   J  journal USB/PipeCam lines, this boot, last 300
 *   L  `lsusb -v` for the camera IDs, if installed
 * Answers are raw; the app redacts.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef ROOTHELPER_H
#define ROOTHELPER_H

int rootHelperMain(int argc, char *argv[]);

#endif // ROOTHELPER_H
