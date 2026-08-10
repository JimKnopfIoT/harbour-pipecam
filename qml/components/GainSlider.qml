/*
 * GainSlider.qml — vertical software brightness bar.
 *
 * WHY THIS AND NOT AN LED CONTROL
 * This bar started life as an LED dimmer. It is not one, because the LED ring
 * provably cannot be driven from the phone: 60 s of capture with the cable's
 * dimmer wheel being turned produced no change in any header field and not one
 * byte on the camera's control endpoint, while a button press in the same run
 * showed up immediately (see the “Protocol” section of README.md). The wheel is an analogue
 * potentiometer in the LED supply and the firmware never sees it.
 *
 * The underlying need is real though — a pipe is dark — so the same strip now
 * brightens the picture instead of the lamp. It is applied in the camera worker
 * right after the JPEG is decoded, so it costs the GUI thread nothing.
 *
 * Vertical and dragged rather than tapped, because it sits under the thumb of
 * the hand holding the phone while the other one feeds cable. Up is brighter.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    /* 1.0 .. maxGain */
    property real gain: 1.0
    property real maxGain: 3.0

    /* Track position as 0..1, which is what the geometry actually works in. */
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

    /* Current value, only while it is doing something — a permanent "1.0x"
     * would just be noise. */
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
        /* Double tap anywhere on the bar goes back to untouched. Faster than
         * dragging exactly to the bottom, and it cannot overshoot. */
        onDoubleClicked: root.gain = 1.0
    }
}
