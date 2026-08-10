# harbour-pipecam — SailfishOS qmake project
#
# A viewer and recorder for USB-C "pipe inspection" endoscope cameras that speak
# the proprietary com.useeplus.protocol (Geek szitman / "supercamera",
# 2ce3:3828). See src/camera/uppprotocol.h for the wire format.
#
# Build:  mb2 build, which wraps mb2 and reports the RPM path.
#         Directly:  mb2 --target SailfishOS-<version>-aarch64 build
#
# The exact target version lives in scripts/lib-common.sh and nowhere else, so
# there is one place to change it — and so this comment does not read like an
# IP address to the anonymity scanner.

TARGET = harbour-pipecam

CONFIG += sailfishapp sailfishapp_i18n c++11

QT += quick

# --- External libraries -----------------------------------------------------
# libusb-1.0   the camera is vendor-class, so uvcvideo never binds it and there
#              is no /dev/videoN — we drive the endpoints ourselves.
# gstreamer    video recording is pure muxing (image/jpeg -> qtmux -> .mp4);
#              gstreamer-app-1.0 provides the appsrc we push JPEGs into.
#
# Do NOT add `CONFIG += link_pkgconfig` here. sailfishapp.prf adds it itself and
# then appends `sailfishapp` to PKGCONFIG. qmake loads CONFIG features in
# reverse order, so naming link_pkgconfig in this file makes it resolve PKGCONFIG
# *before* sailfishapp.prf has contributed its entry — and the link then fails
# with undefined references to SailfishApp::application/createView.
PKGCONFIG += libusb-1.0 gstreamer-1.0 gstreamer-app-1.0

# Bilingual: English source strings + German. libsailfishapp auto-loads the .qm
# matching the device locale (German -> de, otherwise the English source).
TRANSLATIONS += translations/harbour-pipecam-de.ts
lupdate_only {
    SOURCES += qml/*.qml qml/cover/*.qml qml/pages/*.qml
}

# Installed to /usr/share/icons/hicolor/<size>/apps/ (see sailfishapp.prf).
# Regenerate them from icons/icon.svg with ./the icon build step described in README.md.
SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# --- Sources ----------------------------------------------------------------
INCLUDEPATH += \
    src/app \
    src/camera

HEADERS += \
    src/app/capturestore.h \
    src/camera/frameoverlay.h \
    src/camera/mjpegrecorder.h \
    src/camera/uppcamera.h \
    src/camera/uppprotocol.h \
    src/camera/videoframeitem.h

SOURCES += \
    src/app/harbour-pipecam.cpp \
    src/app/capturestore.cpp \
    src/camera/frameoverlay.cpp \
    src/camera/mjpegrecorder.cpp \
    src/camera/uppcamera.cpp \
    src/camera/videoframeitem.cpp

# --- udev rule ---------------------------------------------------------------
# /dev/bus/usb/* is root:usb 0660 and the app user is not in the `usb` group, so
# without this rule libusb cannot open the camera. The rule is scoped to the two
# known VID:PIDs and to the device node only (DEVTYPE=usb_device), so it does
# not loosen anything else on the bus. Same approach harbour-idrone uses for the
# CatSniffer and harbour-sflipper for the Flipper Zero.
#
# The 999- prefix is load-bearing: Sailfish's 999-android-system.rules has a
# catch-all that resets every USB node to 0660 root:usb, and udev's last
# assignment wins. See the header of the rule file for the full explanation.
udevrule.files = data/999-harbour-pipecam-usb.rules
udevrule.path  = /etc/udev/rules.d
INSTALLS += udevrule

# --- QML / assets -----------------------------------------------------------
coverimages.files = qml/images/cover-logo.png
coverimages.path  = /usr/share/$${TARGET}/qml/images
INSTALLS += coverimages

OTHER_FILES += \
    qml/harbour-pipecam.qml \
    qml/cover/CoverPage.qml \
    qml/pages/ViewfinderPage.qml \
    qml/pages/GalleryPage.qml \
    qml/pages/CaptureViewPage.qml \
    qml/pages/RenameDialog.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/SpecsPage.qml \
    qml/pages/AboutPage.qml \
    qml/components/ShutterButton.qml \
    qml/components/RecordButton.qml \
    qml/components/GainSlider.qml \
    qml/components/RollIndicator.qml \
    qml/components/StatusOverlay.qml \
    harbour-pipecam.desktop \
    rpm/harbour-pipecam.spec \
    data/999-harbour-pipecam-usb.rules

DISTFILES += README.md
