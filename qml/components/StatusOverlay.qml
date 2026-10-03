/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
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
            return camera.statusDetail
        }
    }
}
