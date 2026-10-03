/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
/* Software gain: the LED dimmer is analogue, not reachable over USB. */
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    /* 1.0 .. maxGain */
    property real gain: 1.0
    property real maxGain: 3.0

    readonly property real fraction: (gain - 1.0) / Math.max(0.001, maxGain - 1.0)

    implicitWidth: Theme.itemSizeSmall

    Rectangle {
        id: track
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: Theme.paddingMedium
        radius: width / 2
        color: Qt.rgba(0, 0, 0, 0.45)
        border.width: 1
        border.color: Theme.rgba(Theme.lightSecondaryColor, 0.5)

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: parent.height * root.fraction
            radius: parent.radius
            color: "#FFC061"
        }
    }

    Rectangle {
        id: handle
        width: Theme.itemSizeExtraSmall * 0.62
        height: width
        radius: width / 2
        anchors.horizontalCenter: track.horizontalCenter
        y: track.y + (track.height - height) * (1 - root.fraction)
        color: "#FFD98A"
        border.width: 2
        border.color: Qt.rgba(0, 0, 0, 0.5)
    }

    Label {
        anchors {
            bottom: track.top
            bottomMargin: Theme.paddingSmall
            horizontalCenter: parent.horizontalCenter
        }
        visible: root.gain > 1.01
        text: root.gain.toFixed(1) + "×"
        font.pixelSize: Theme.fontSizeExtraSmall
        color: "#FFD98A"
        style: Text.Outline
        styleColor: Qt.rgba(0, 0, 0, 0.8)
    }

    MouseArea {
        anchors.fill: parent

        function setFromY(y) {
            var t = 1 - (y - track.y) / track.height
            t = Math.max(0, Math.min(1, t))
            root.gain = 1.0 + t * (root.maxGain - 1.0)
        }

        onPressed: setFromY(mouse.y)
        onPositionChanged: setFromY(mouse.y)
        onDoubleClicked: root.gain = 1.0
    }
}
