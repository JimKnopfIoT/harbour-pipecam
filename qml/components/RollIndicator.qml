/*
 * RollIndicator.qml — shows how far the picture is rotated, and puts it back.
 *
 * A camera head sliding down a pipe twists as it goes, so the picture arrives at
 * an arbitrary angle. Two-finger rotation on the viewfinder turns it back, but a
 * rotated image gives you no reference for how far you have turned it — after a
 * few corrections you no longer know which way is actually up.
 *
 * Hence a dial: a fixed ring with a dot that rides around it, plus a small tick
 * at the top marking level. The dot is where "up in the picture" currently
 * points.
 *
 * IT IS ALSO THE CONTROL, not just a readout. Drag anywhere on the ring and the
 * picture follows your finger; tap the middle and it snaps back to level. That
 * matters because the two-finger twist on the viewfinder is a fiddly gesture to
 * land while holding a phone in one hand and ten metres of cable in the other —
 * and if it does not register, a dial that only *displays* the angle leaves you
 * with no way to fix the picture at all. One-finger drag always works.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
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

    /* Ring */
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

    /* Level mark at twelve o'clock — the reference the dot is read against. */
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

    /* The dot. Placed by rotating a container rather than by computing sin/cos
     * per frame: one rotation binding is cheaper to read and cannot drift out of
     * step with the ring. */
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

    /* Angle, only when it is not zero. */
    Label {
        anchors.centerIn: ring
        visible: Math.abs(root.roll) > 0.5
        text: Math.round(root.roll) + "°"
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.lightPrimaryColor
        style: Text.Outline
        styleColor: Qt.rgba(0, 0, 0, 0.8)
    }

    /* Hint that the middle is a button, once the angle is off level. */
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

        /* Inside this radius a press means "reset", outside it means "turn".
         * Half the ring is a generous target for a thumb and still leaves a
         * wide annulus to drag on. */
        readonly property real innerFraction: 0.5
        property bool turning: false

        function radiusOf(x, y) {
            var dx = x - width / 2
            var dy = y - height / 2
            return Math.sqrt(dx * dx + dy * dy) / (width / 2)
        }

        /* Angle of the touch point measured from twelve o'clock, clockwise —
         * the same convention the dot is drawn with, so the dot lands exactly
         * under the finger. */
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
            /* A drag that started in the centre still turns the dial once it
             * leaves it — otherwise a slightly off-centre grab feels dead. */
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
