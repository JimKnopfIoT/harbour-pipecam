/*
 * SettingsPage.qml — the handful of things worth making adjustable.
 *
 * Everything here exists because the hardware or the job demands it, not for
 * completeness: the camera head can be fed into a pipe upside down (mirror),
 * a round pipe rarely fills a 4:3 frame the way you want (fill mode), the
 * screen must not blank while you stare at it, and the only control you can
 * reach with ten metres of cable out is the button on it.
 *
 * Settings persist in dconf under /apps/harbour-pipecam/ via the
 * ConfigurationGroup in harbour-pipecam.qml.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.Landscape

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            /* These three used to live in the viewfinder's pulley menu. That
             * menu had to go: its pull-down gesture is the same gesture the
             * viewfinder needs for panning the magnified image, so the two
             * could never both work. */
            SectionHeader { text: qsTr("Camera") }

            TextSwitch {
                text: qsTr("Camera on")
                description: app.camera.statusText
                     + (app.camera.statusDetail !== ""
                        ? " — " + app.camera.statusDetail : "")
                checked: app.camera.running
                onClicked: app.camera.running ? app.camera.stop() : app.camera.start()
            }

            TextSwitch {
                text: qsTr("Show date and time")
                description: qsTr("Shown in the corner of the picture and burnt "
                                  + "into snapshots and recordings. Burning it "
                                  + "in means each frame has to be re-encoded "
                                  + "instead of being saved exactly as the "
                                  + "camera sent it.")
                checked: app.settings.showTimestamp
                onClicked: app.settings.showTimestamp = !app.settings.showTimestamp
            }

            SectionHeader { text: qsTr("Image") }

            TextSwitch {
                text: qsTr("Mirror image")
                description: qsTr("Flip left to right. Useful when the camera "
                                  + "head is fed into the pipe rotated.")
                checked: app.settings.mirrored
                onClicked: app.settings.mirrored = !app.settings.mirrored
            }

            ComboBox {
                label: qsTr("Scaling")
                description: qsTr("How the 4:3 camera image fills the screen.")
                currentIndex: app.settings.fillMode
                menu: ContextMenu {
                    MenuItem { text: qsTr("Fit — show the whole frame") }
                    MenuItem { text: qsTr("Fill — crop the edges") }
                    MenuItem { text: qsTr("Stretch — distort to fit") }
                }
                onCurrentIndexChanged: app.settings.fillMode = currentIndex
            }

            TextSwitch {
                text: qsTr("Save rotated")
                description: qsTr("Apply the roll dial to snapshots and "
                                  + "recordings, not only to the live view. The "
                                  + "picture is scaled to fit, so nothing is cut "
                                  + "off — turning it leaves black corners.")
                checked: app.settings.captureRotated
                onClicked: app.settings.captureRotated = !app.settings.captureRotated
            }

            TextSwitch {
                text: qsTr("Show grid")
                description: qsTr("Thirds overlay — helps to judge whether the "
                                  + "camera head is centred in the pipe.")
                checked: app.settings.showGrid
                onClicked: app.settings.showGrid = !app.settings.showGrid
            }

            SectionHeader { text: qsTr("Camera cable") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The dimmer wheel is wired straight to the LEDs and "
                           + "cannot be read or driven from the phone (measured "
                           + "— see the specs page). The bar on the left of the "
                           + "viewfinder brightens the picture instead.")
            }

            ComboBox {
                label: qsTr("Button on the cable")
                description: qsTr("What the push-button on the camera cable does. "
                                  + "Turn it off if you keep catching it while "
                                  + "using the dimmer wheel next to it.")
                /* Index order must match the menu below and the mapping in
                 * onCurrentIndexChanged. */
                currentIndex: app.settings.cableButtonAction === "record" ? 1
                            : app.settings.cableButtonAction === "off" ? 2
                            : 0
                menu: ContextMenu {
                    MenuItem { text: qsTr("Take a snapshot") }
                    MenuItem { text: qsTr("Start / stop recording") }
                    MenuItem { text: qsTr("Do nothing") }
                }
                onCurrentIndexChanged: {
                    app.settings.cableButtonAction =
                        currentIndex === 1 ? "record"
                      : currentIndex === 2 ? "off"
                      : "snapshot"
                }
            }

            SectionHeader { text: qsTr("Behaviour") }

            TextSwitch {
                text: qsTr("Keep the display on")
                description: qsTr("Prevents blanking while the camera is live. "
                                  + "Only active when frames are arriving.")
                checked: app.settings.keepDisplayOn
                onClicked: app.settings.keepDisplayOn = !app.settings.keepDisplayOn
            }

            SectionHeader { text: qsTr("Storage") }

            DetailItem {
                label: qsTr("Photos")
                value: app.captures.pictureDir
            }
            DetailItem {
                label: qsTr("Videos")
                value: app.captures.videoDir
            }

            SectionHeader { text: qsTr("Captures") }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Open the gallery")
                onClicked: pageStack.push(Qt.resolvedUrl("GalleryPage.qml"))
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Camera specs")
                onClicked: pageStack.push(Qt.resolvedUrl("SpecsPage.qml"))
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("About PipeCam")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}
