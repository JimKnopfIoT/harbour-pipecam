/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: root

    width: Theme.itemSizeLarge
    height: width

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
        id: innerDisc
        anchors.centerIn: parent
        width: parent.width * (root.pressedDown ? 0.66 : 0.78)
        height: width
        radius: width / 2
        color: root.enabled ? Theme.rgba(Theme.lightPrimaryColor, 0.95)
                            : Theme.rgba(Theme.lightSecondaryColor, 0.3)
        Behavior on width { NumberAnimation { duration: 90 } }
    }
}
