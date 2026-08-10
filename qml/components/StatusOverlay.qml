/*
 * StatusOverlay.qml — what the user sees instead of a picture.
 *
 * This is the app's most important screen when things go wrong, so it does not
 * just say "no signal": it names the state and, where the cause is knowable,
 * says what to do about it. The three realistic failures are a cable that is
 * not plugged in, a camera the app is not permitted to open (missing udev
 * rule), and a camera that is simply still connecting.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.pipecam 1.0

Column {
    id: root

    property var camera

    spacing: Theme.paddingLarge

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        source: camera && camera.status === PipeCamera.Error
                ? "image://theme/icon-l-attention"
                : "image://theme/icon-l-image"
        opacity: 0.5

        /* Slow breathing while we are actively looking, so the screen reads as
         * "working on it" rather than "hung". */
        SequentialAnimation on opacity {
            running: camera && camera.running && camera.status !== PipeCamera.Error
            loops: Animation.Infinite
            NumberAnimation { to: 0.2; duration: 900; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 0.5; duration: 900; easing.type: Easing.InOutQuad }
        }
    }

    Label {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: camera ? camera.statusText : ""
        color: Theme.lightPrimaryColor
        font.pixelSize: Theme.fontSizeLarge
    }

    Label {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: text !== ""
        color: Theme.lightSecondaryColor
        font.pixelSize: Theme.fontSizeSmall
        text: {
            if (!camera)
                return ""
            if (!camera.running)
                return qsTr("Open settings with the gear button and switch the "
                            + "camera on.")
            /* statusDetail carries the actionable message from the backend —
             * "check the USB-C connection", "permission rule is missing", … */
            return camera.statusDetail
        }
    }
}
