/*
 * RootConfirmDialog.qml — the one deliberate step before the root helper runs.
 *
 * Not an authentication: an unprivileged app cannot ask the device lock for
 * one on Sailfish, and the helper unit is startable by any process of this
 * user anyway (that is what the polkit rule says). What the dialog buys is
 * that nothing starts it silently, and that the user knows what it reads.
 * Same reasoning as harbour-sysmetrics.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog
    allowedOrientations: Orientation.Landscape

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height + Theme.paddingLarge

        Column {
            id: col
            width: parent.width
            spacing: Theme.paddingMedium

            DialogHeader { acceptText: qsTr("Start helper") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                text: qsTr("This starts a small helper service running as root. "
                           + "It only reads, and only these things:")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("• which processes, of any user, have the camera open\n"
                           + "• the kernel's USB table entry for the camera\n"
                           + "• kernel log and journal lines that mention USB\n"
                           + "• lsusb output for the camera, if lsusb is installed")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Log lines come from the whole system. Addresses and "
                           + "identifiers are removed, but read the report before "
                           + "passing it on. The helper stops when you leave the "
                           + "report page and is never started at boot.")
            }
        }
        VerticalScrollDecorator {}
    }
}
