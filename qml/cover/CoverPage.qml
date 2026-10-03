/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Image {
        anchors.fill: parent
        source: Qt.resolvedUrl("../images/cover-logo.png")
        fillMode: Image.PreserveAspectCrop
        opacity: 0.38
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.35) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.7) }
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeSmall + Theme.paddingLarge
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "PipeCam"
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.lightPrimaryColor
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingSmall

            Rectangle {
                width: Theme.paddingSmall
                height: width
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                color: app.recorder.recording ? "#E4382E"
                     : app.camera.streaming ? "#5FD35F"
                     : Theme.lightSecondaryColor
                SequentialAnimation on opacity {
                    running: app.recorder.recording
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 500 }
                    NumberAnimation { to: 1.0; duration: 500 }
                }
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.lightSecondaryColor
                text: {
                    if (app.recorder.recording) {
                        var total = Math.floor(app.recorder.durationMs / 1000)
                        var m = Math.floor(total / 60)
                        var s = total % 60
                        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
                    }
                    return app.camera.statusText
                }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: app.captures.count > 0 && !app.recorder.recording
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.lightSecondaryColor
            /* No //% id: qsTrId syntax, lupdate ignores it above qsTr(). */
            text: qsTr("%n capture(s)", "", app.captures.count)
        }
    }

    CoverActionList {
        id: coverActions

        CoverAction {
            iconSource: "image://theme/icon-cover-camera"
            onTriggered: app.takeSnapshot()
        }
        CoverAction {
            /* No stock stop icon; pause glyph while recording. */
            iconSource: app.recorder.recording ? "image://theme/icon-cover-pause"
                                               : "image://theme/icon-cover-new"
            onTriggered: app.toggleRecording()
        }
    }
}
