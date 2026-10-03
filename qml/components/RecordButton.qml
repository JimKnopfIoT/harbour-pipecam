/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
/* Same size as ShutterButton; distinguished by shape. */
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: root

    width: Theme.itemSizeLarge
    height: width

    property bool recording: false
    property bool pressedDown: pressed && containsMouse

    Rectangle {
        id: outerRing
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 4
        border.color: root.enabled ? Theme.rgba(Theme.lightPrimaryColor, 0.95)
                                   : Theme.rgba(Theme.lightSecondaryColor, 0.3)
    }

    Rectangle {
        id: inner
        anchors.centerIn: parent
        width: parent.width * (root.recording ? 0.46
                                              : (root.pressedDown ? 0.62 : 0.72))
        height: width
        radius: root.recording ? Theme.paddingSmall / 2 : width / 2
        color: root.enabled ? "#E4382E" : Theme.rgba("#E4382E", 0.3)

        Behavior on width  { NumberAnimation { duration: 140 } }
        Behavior on radius { NumberAnimation { duration: 140 } }
    }
}
