# Neutral packaging metadata — no personal identifiers (see CLAUDE.md anonymity rules).
Name:       harbour-pipecam
Summary:    Viewer and recorder for USB pipe inspection cameras
Version:    0.1.0
Release:    1
# ANONYMITY: neutral build host so built RPMs carry no real hostname/domain.
# Without this, the RPM BUILDHOST tag leaks the build machine's name and LAN
# domain into every published package.
%define _buildhost reproducible-builder
License:    GPL-3.0-or-later
URL:        https://github.com/JimKnopfIoT/harbour-pipecam
Source0:    %{name}-%{version}.tar.bz2
Vendor:     harbour-pipecam contributors
Packager:   harbour-pipecam contributors

Requires:   sailfishsilica-qt5
# libusb and the gstreamer core arrive automatically as soname dependencies of
# the binary, so they are not listed here. qtmux does NOT: it is a plugin that
# gstreamer dlopens at runtime, so nothing links against it and rpm cannot infer
# it. Without this line video recording would fail at the moment the user first
# presses record, on a device where everything else works.
Requires:   gstreamer1.0-plugins-good
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(libusb-1.0)
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(gstreamer-app-1.0)
BuildRequires: desktop-file-utils

%description
Live view, snapshots and video recording for USB-C endoscope / pipe inspection
cameras of the "com.useeplus.protocol" family (sold as USeePlus, Geek szitman,
supercamera). These cameras announce themselves as vendor-class devices rather
than UVC, so no kernel driver binds them and no /dev/video node appears;
PipeCam speaks their protocol directly over libusb.

Snapshots are written as the camera's own untouched JPEG bytes and recordings
are muxed straight into MP4, so nothing is ever re-encoded. Everything runs
on-device: no network access, no telemetry.

%prep
%setup -q

%build
%qmake5
%make_build

%install
%qmake5_install

%post
# The udev rule grants the app access to the camera's raw USB node. Reload the
# rules so it applies without a reboot; a camera plugged in before installation
# still needs a replug to pick up the new mode.
if [ -x /sbin/udevadm ] || [ -x /usr/bin/udevadm ]; then
    udevadm control --reload-rules >/dev/null 2>&1 || :
    udevadm trigger --subsystem-match=usb >/dev/null 2>&1 || :
fi

%postun
if [ $1 -eq 0 ]; then
    if [ -x /sbin/udevadm ] || [ -x /usr/bin/udevadm ]; then
        udevadm control --reload-rules >/dev/null 2>&1 || :
    fi
fi

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%config %{_sysconfdir}/udev/rules.d/999-harbour-pipecam-usb.rules

%changelog
* Mon Aug 10 2026 harbour-pipecam contributors 0.1.0-1
- First release: live view, snapshots, MJPEG-to-MP4 recording, gallery.
