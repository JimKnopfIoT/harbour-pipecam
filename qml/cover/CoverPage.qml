/*
 * CoverPage.qml — the tile shown while the app runs in the background.
 *
 * The background is qml/images/cover-logo.png: a hard zoom into the app icon's
 * lens and inner rings, generated from icons/icon.svg by the icon build step described in README.md.
 * The full icon has five rings and would turn to mush at cover size; the crop
 * keeps the same motif instantly recognisable while leaving room for the state
 * on top of it.
 *
 * The cover earns its place by being useful, not decorative: it shows whether
 * frames are still arriving and how long a recording has been running, and its
 * two actions are the two things worth doing without opening the app.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Image {
        anchors.fill: parent
        source: Qt.resolvedUrl("../images/cover-logo.png")
        fillMode: Image.PreserveAspectCrop
        /* Dimmed hard: this is a backdrop, and the text has to win. */
        opacity: 0.38
        asynchronous: true
    }

    /* Darken towards the bottom so the status block always has contrast,
     * whatever part of the artwork ends up behind it. */
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
            /* No //% id comment here: that syntax belongs to qsTrId(), and
             * lupdate warns and ignores it when it sits above a qsTr(). */
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
            /* No stock "stop recording" cover icon exists, so reuse the pause
             * glyph for the running state — it reads as "end this". */
            iconSource: app.recorder.recording ? "image://theme/icon-cover-pause"
                                               : "image://theme/icon-cover-new"
            onTriggered: app.toggleRecording()
        }
    }
}
