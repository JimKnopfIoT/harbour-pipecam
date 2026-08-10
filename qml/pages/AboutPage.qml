/*
 * AboutPage.qml — what the app is, what hardware it speaks to, and the licence.
 *
 * The hardware section is not decoration: these cameras are sold under a dozen
 * names with no documentation, so telling the user exactly which USB IDs are
 * supported is the fastest way for them to work out whether their cable will
 * ever work.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.Landscape

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("About PipeCam") }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "image://theme/harbour-pipecam"
                width: Theme.iconSizeLarge
                height: width
                fillMode: Image.PreserveAspectFit
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Live view, snapshots and recording for USB-C pipe "
                           + "inspection cameras.")
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }

            SectionHeader { text: qsTr("Supported hardware") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Endoscope cameras that speak the “com.useeplus” "
                           + "protocol, sold as USeePlus, Geek szitman or "
                           + "supercamera:\n\n"
                           + "    USB 2ce3:3828\n"
                           + "    USB 0329:2022\n\n"
                           + "These cameras are not UVC devices, so no kernel "
                           + "driver binds them and they never appear as a "
                           + "/dev/video node. PipeCam drives their USB "
                           + "endpoints directly.")
            }

            SectionHeader { text: qsTr("How captures are stored") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("The camera streams MJPEG, so a snapshot is written "
                           + "as the camera's own untouched JPEG and a recording "
                           + "is muxed straight into MP4 — nothing is re-encoded. "
                           + "Switching on the timestamp or the brightness gain "
                           + "does force a re-encode, because both change the "
                           + "picture. No location or device information is "
                           + "added to any file.")
            }

            SectionHeader { text: qsTr("Privacy") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Everything runs on the device. PipeCam contains no "
                           + "network code at all: there is no telemetry, no "
                           + "cloud component and nothing to opt out of.")
            }

            SectionHeader { text: qsTr("Licence") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("GNU General Public License v3.0 or later. "
                           + "Provided as is, with no warranty.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: "https://github.com/JimKnopfIoT/harbour-pipecam"
            }
        }

        VerticalScrollDecorator {}
    }
}
