TARGET = harbour-pipecam

CONFIG += sailfishapp sailfishapp_i18n c++11

# network: QLocalSocket to root helper only; dbus: systemd unit start.
QT += quick network dbus

# PIPECAM_VERSION is set by the spec; plain qmake: "dev".
!contains(DEFINES, PIPECAM_VERSION=.*): DEFINES += PIPECAM_VERSION=dev

# No CONFIG += link_pkgconfig: resolves PKGCONFIG before sailfishapp.prf adds
# its entry -> undefined SailfishApp::application/createView.
PKGCONFIG += libusb-1.0 gstreamer-1.0 gstreamer-app-1.0

TRANSLATIONS += translations/harbour-pipecam-de.ts
lupdate_only {
    SOURCES += qml/*.qml qml/cover/*.qml qml/pages/*.qml
}

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

INCLUDEPATH += \
    src/app \
    src/camera \
    src/diag

HEADERS += \
    src/app/capturestore.h \
    src/camera/frameoverlay.h \
    src/camera/mjpegrecorder.h \
    src/camera/uppcamera.h \
    src/camera/uppprotocol.h \
    src/camera/uppvariant.h \
    src/camera/videoframeitem.h \
    src/diag/diaglog.h \
    src/diag/diagreport.h \
    src/diag/redact.h \
    src/diag/rootclient.h \
    src/diag/usertext.h \
    src/diag/roothelper.h \
    src/diag/usbdump.h

SOURCES += \
    src/app/harbour-pipecam.cpp \
    src/app/capturestore.cpp \
    src/camera/frameoverlay.cpp \
    src/camera/mjpegrecorder.cpp \
    src/camera/uppcamera.cpp \
    src/camera/videoframeitem.cpp \
    src/diag/diaglog.cpp \
    src/diag/diagreport.cpp \
    src/diag/redact.cpp \
    src/diag/rootclient.cpp \
    src/diag/roothelper.cpp \
    src/diag/usbdump.cpp

# 999- prefix required, see rule file.
udevrule.files = data/999-harbour-pipecam-usb.rules
udevrule.path  = /etc/udev/rules.d
INSTALLS += udevrule

# Never enabled; started on demand, polkit allows this unit for defaultuser only.
helperservice.files = data/harbour-pipecam-helper.service
helperservice.path  = /usr/lib/systemd/system
INSTALLS += helperservice

polkitrule.files = data/50-harbour-pipecam.rules
polkitrule.path  = /usr/share/polkit-1/rules.d
INSTALLS += polkitrule

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
    qml/pages/DiagReportPage.qml \
    qml/pages/RootConfirmDialog.qml \
    qml/components/ShutterButton.qml \
    qml/components/RecordButton.qml \
    qml/components/GainSlider.qml \
    qml/components/RollIndicator.qml \
    qml/components/StatusOverlay.qml \
    qml/components/LinkRow.qml \
    harbour-pipecam.desktop \
    rpm/harbour-pipecam.spec \
    data/999-harbour-pipecam-usb.rules \
    data/harbour-pipecam-helper.service \
    data/50-harbour-pipecam.rules

DISTFILES += README.md
