/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    /* Degrees; 0 is level. */
    property real roll: 0
    signal resetRequested()
    signal rollRequested(real degrees)

    implicitWidth: Theme.itemSizeLarge
    implicitHeight: Theme.itemSizeLarge

    Rectangle {
        id: ring
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        color: Qt.rgba(0, 0, 0, 0.45)
        border.width: 2
        border.color: Theme.rgba(Theme.lightPrimaryColor,
                                 Math.abs(root.roll) > 0.5 ? 0.85 : 0.4)
    }

    Rectangle {
        width: 2
        height: ring.width * 0.16
        color: Theme.rgba(Theme.lightSecondaryColor, 0.8)
        anchors {
            horizontalCenter: ring.horizontalCenter
            top: ring.top
            topMargin: 3
        }
    }

    Item {
        anchors.centerIn: ring
        width: ring.width
        height: ring.height
        rotation: root.roll

        Rectangle {
            width: ring.width * 0.19
            height: width
            radius: width / 2
            color: "#FFC061"
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
                topMargin: ring.width * 0.055
            }
        }
    }

    Label {
        anchors.centerIn: ring
        visible: Math.abs(root.roll) > 0.5
        text: Math.round(root.roll) + "°"
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.lightPrimaryColor
        style: Text.Outline
        styleColor: Qt.rgba(0, 0, 0, 0.8)
    }

    Rectangle {
        anchors.centerIn: ring
        width: ring.width * 0.5
        height: width
        radius: width / 2
        color: "transparent"
        border.width: 1
        border.color: Theme.rgba(Theme.lightSecondaryColor, 0.35)
        visible: Math.abs(root.roll) > 0.5
    }

    MouseArea {
        id: dial
        anchors.fill: ring

        readonly property real innerFraction: 0.5
        property bool turning: false

        function radiusOf(x, y) {
            var dx = x - width / 2
            var dy = y - height / 2
            return Math.sqrt(dx * dx + dy * dy) / (width / 2)
        }

        /* Clockwise from 12 o'clock, same convention as the dot. */
        function angleOf(x, y) {
            var dx = x - width / 2
            var dy = y - height / 2
            return Math.atan2(dx, -dy) * 180 / Math.PI
        }

        onPressed: {
            turning = radiusOf(mouse.x, mouse.y) >= innerFraction
            if (turning)
                root.rollRequested(angleOf(mouse.x, mouse.y))
        }

        onPositionChanged: {
            if (!turning && radiusOf(mouse.x, mouse.y) >= innerFraction)
                turning = true
            if (turning)
                root.rollRequested(angleOf(mouse.x, mouse.y))
        }

        onClicked: {
            if (!turning)
                root.resetRequested()
        }
    }
}
