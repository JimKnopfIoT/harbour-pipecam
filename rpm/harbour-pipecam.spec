Name:       harbour-pipecam
Summary:    Viewer and recorder for USB pipe inspection cameras
Version:    0.1.3
Release:    1
# Neutral BUILDHOST tag instead of the build machine's hostname.
%define _buildhost reproducible-builder
License:    GPL-3.0-or-later
URL:        https://github.com/JimKnopfIoT/harbour-pipecam
Source0:    %{name}-%{version}.tar.bz2
Vendor:     harbour-pipecam contributors
Packager:   harbour-pipecam contributors

Requires:   sailfishsilica-qt5
# qtmux: dlopened plugin, not an auto soname dependency.
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
%qmake5 "DEFINES+=PIPECAM_VERSION=%{version}-%{release}"
%make_build

%install
%qmake5_install

%post
# Already plugged camera needs a replug.
if [ -x /sbin/udevadm ] || [ -x /usr/bin/udevadm ]; then
    udevadm control --reload-rules >/dev/null 2>&1 || :
    udevadm trigger --subsystem-match=usb >/dev/null 2>&1 || :
fi
systemctl daemon-reload >/dev/null 2>&1 || :

%preun
if [ $1 -eq 0 ]; then
    systemctl stop harbour-pipecam-helper.service >/dev/null 2>&1 || :
fi

%postun
systemctl daemon-reload >/dev/null 2>&1 || :
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
/usr/lib/systemd/system/harbour-pipecam-helper.service
%{_datadir}/polkit-1/rules.d/50-harbour-pipecam.rules

%changelog
* Sat Oct 03 2026 harbour-pipecam contributors 0.1.3-1
- Support for the single-interface variant of the camera (bcdDevice 1.11,
  raw 320x240 YUYV instead of MJPEG), detected from its USB descriptor.
  Thanks to the reporters on OpenRepos and GitHub (#1).
- An unknown variant is named as such instead of failing with a USB error.
- Diagnostic report states the detected variant; its claim test only tries
  the interfaces that variant uses.
- The log is always in English, whatever the UI language.

* Fri Oct 02 2026 harbour-pipecam contributors 0.1.2-1
- Diagnostic report (Settings -> About): app, system and full USB details of
  the camera, anonymised so it can be posted as it is. Optional root helper
  adds the kernel's view: who holds the camera, kernel log, journal.
- Verbose log switch on the About page, kept across a crash.
- "Camera is busy" no longer covers every failure: each USB error has its own
  message and shows the libusb code.
- About page: the GitHub link opens the repository; new link to the issues.

* Sun Aug 30 2026 harbour-pipecam contributors 0.1.1-1
- Jolla J2 (and any phone with a camera cutout): the viewfinder now fills the
  whole screen instead of leaving a strip of desktop beside the notch, and the
  settings/brightness column steps aside so no control sits under the cutout.

* Mon Aug 10 2026 harbour-pipecam contributors 0.1.0-1
- First release: live view, snapshots, MJPEG-to-MP4 recording, gallery.
