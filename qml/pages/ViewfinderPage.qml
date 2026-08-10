/*
 * ViewfinderPage.qml — the live view and the capture controls.
 *
 * LAYOUT
 * The picture is the app; everything else floats over it. The app is
 * landscape-locked, so the screen is short and wide, and the controls live in
 * two narrow columns down the left and right edges rather than in a bar along
 * the bottom, which would eat a third of the picture.
 *
 *   right : photo (top), video, gallery (bottom)  — the two capture buttons are
 *           the same size and sit together where the thumb rests
 *   left  : settings gear, level with the photo button, and the LED brightness
 *           bar below it
 *
 * They stay permanently visible: on a job you need to know without looking that
 * the shutter is where you left it.
 *
 * WHY THERE IS NO PULLEY MENU
 * There was one, and it could not be operated. The viewfinder needs a MouseArea
 * across the picture to drag the magnified image around, and that MouseArea
 * necessarily swallows the downward drag a PullDownMenu needs to open — the two
 * gestures are the same gesture. Rather than fight it with filtering rules that
 * would make both feel unreliable, the menu became the gear button on the left,
 * and the pan/zoom area is now anchored strictly BETWEEN the two control
 * columns, so it can never eat a press meant for a button.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.pipecam 1.0

import "../components"

Page {
    id: page

    /* Landscape only — see the rationale in harbour-pipecam.qml. */
    allowedOrientations: Orientation.Landscape

    /* ------------------------------------------------------------------ */
    /* Live image                                                          */
    /* ------------------------------------------------------------------ */
    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Viewfinder {
        id: viewfinder
        anchors.fill: parent
        camera: app.camera
        mirrored: app.settings.mirrored
        fillMode: app.settings.fillMode
        /* In Fit mode the roll shrink already keeps the picture inside, but
         * Fill and Stretch let a rolled frame overhang; clip so it never paints
         * over the control columns. */
        clip: true
        /* Publish the roll upward so the recorder and the snapshot path can
         * apply it too — see captureRoll in harbour-pipecam.qml. */
        onRollChanged: app.viewRoll = roll
        Component.onCompleted: roll = app.viewRoll
    }

    /* Composition grid — thirds. Useful for judging whether the camera head is
     * running centred in the pipe. */
    Item {
        anchors.fill: parent
        visible: app.settings.showGrid && viewfinder.hasFrame
        opacity: 0.35
        Repeater {
            model: 2
            Rectangle {
                width: 1; height: parent.height
                x: parent.width * (index + 1) / 3
                color: Theme.lightPrimaryColor
            }
        }
        Repeater {
            model: 2
            Rectangle {
                height: 1; width: parent.width
                y: parent.height * (index + 1) / 3
                color: Theme.lightPrimaryColor
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Pinch to zoom, drag to pan — strictly between the two columns        */
    /* ------------------------------------------------------------------ */
    PinchArea {
        id: pinchArea
        anchors {
            left: leftBar.right
            right: controlBar.left
            top: parent.top
            bottom: parent.bottom
        }
        /* PINCH IS ZOOM ONLY — rotation is deliberately NOT handled here.
         *
         * It was, and it worked, but it fought the zoom: the two gestures share
         * the same two fingers, and it is nearly impossible to change the scale
         * without also twisting a few degrees, or to twist without nudging the
         * scale. Every zoom left the picture slightly crooked. The roll dial in
         * the left column does the job with one finger and cannot be triggered
         * by accident, so the pinch keeps the single meaning it is good at.
         *
         * (Note that PinchArea reports no rotation at all unless
         * pinch.minimumRotation/maximumRotation are set — leaving them out is
         * what disables it, not an oversight.) */
        pinch.minimumScale: 1.0
        pinch.maximumScale: viewfinder.maxZoom

        property real startZoom: 1.0
        onPinchStarted: startZoom = viewfinder.zoom
        onPinchUpdated: viewfinder.zoom = startZoom * pinch.scale

        MouseArea {
            anchors.fill: parent

            property real lastX: 0
            property real lastY: 0
            property bool dragging: false

            onPressed: {
                lastX = mouse.x
                lastY = mouse.y
                dragging = false
            }

            onPositionChanged: {
                if (!viewfinder.canPan())
                    return
                var dx = mouse.x - lastX
                var dy = mouse.y - lastY
                /* Ignore the first few pixels so a slightly sloppy tap stays a
                 * tap and does not become a one-pixel pan. */
                if (!dragging && Math.abs(dx) + Math.abs(dy) < 8)
                    return
                dragging = true
                viewfinder.panBy(dx, dy)
                lastX = mouse.x
                lastY = mouse.y
            }

            onDoubleClicked: {
                /* Cycle 1x → 2x → max → 1x, for when a pinch is awkward
                 * one-handed. Returning to 1x recentres, since at 1x there is
                 * nothing to pan to anyway. */
                if (viewfinder.zoom >= viewfinder.maxZoom) {
                    viewfinder.zoom = 1.0
                    viewfinder.resetPan()
                } else if (viewfinder.zoom >= 2.0) {
                    viewfinder.zoom = viewfinder.maxZoom
                } else {
                    viewfinder.zoom = 2.0
                }
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Placeholder while there is no picture                                */
    /* ------------------------------------------------------------------ */
    StatusOverlay {
        anchors.horizontalCenter: pinchArea.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: pinchArea.width - 2 * Theme.paddingLarge
        camera: app.camera
        visible: !viewfinder.hasFrame
    }

    /* ------------------------------------------------------------------ */
    /* Top status strip                                                    */
    /* ------------------------------------------------------------------ */
    Item {
        id: topBar
        anchors { top: parent.top; left: leftBar.right; right: controlBar.left }
        height: Theme.itemSizeSmall

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.55) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Row {
            anchors {
                left: parent.left
                leftMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingMedium

            Rectangle {
                width: Theme.paddingSmall
                height: width
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                color: app.camera.streaming ? "#5FD35F"
                     : app.camera.status === PipeCamera.Error ? Theme.errorColor
                     : Theme.highlightColor
                /* Pulse while searching, so "no camera" never looks frozen. */
                SequentialAnimation on opacity {
                    running: !app.camera.streaming && app.camera.running
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.25; duration: 600 }
                    NumberAnimation { to: 1.0;  duration: 600 }
                }
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: app.camera.statusText
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.lightPrimaryColor
            }
        }

        Row {
            anchors {
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            spacing: Theme.paddingMedium

            Label {
                anchors.verticalCenter: parent.verticalCenter
                visible: viewfinder.zoom > 1.01
                text: viewfinder.zoom.toFixed(1) + "×"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.lightSecondaryColor
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                visible: app.camera.streaming
                text: qsTr("%1 fps").arg(app.camera.fps.toFixed(0))
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.lightSecondaryColor
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Recording indicator                                                 */
    /* ------------------------------------------------------------------ */
    Rectangle {
        id: recIndicator
        visible: app.recorder.recording
        anchors {
            top: parent.top
            topMargin: Theme.itemSizeSmall + Theme.paddingMedium
            horizontalCenter: pinchArea.horizontalCenter
        }
        width: recRow.width + 2 * Theme.paddingLarge
        height: recRow.height + Theme.paddingMedium
        radius: height / 2
        color: Qt.rgba(0, 0, 0, 0.6)

        Row {
            id: recRow
            anchors.centerIn: parent
            spacing: Theme.paddingMedium

            Rectangle {
                width: Theme.paddingMedium
                height: width
                radius: width / 2
                color: "#E4382E"
                anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity {
                    running: app.recorder.recording
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 500 }
                    NumberAnimation { to: 1.0; duration: 500 }
                }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: page.formatDuration(app.recorder.durationMs)
                color: Theme.lightPrimaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: (app.recorder.bytesWritten / (1024 * 1024)).toFixed(0) + " MB"
                color: Theme.lightSecondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* Burnt-in timestamp, bottom right of the picture                     */
    /* ------------------------------------------------------------------ */
    Label {
        visible: app.settings.showTimestamp
        anchors {
            right: controlBar.left
            bottom: parent.bottom
            margins: Theme.paddingMedium
        }
        text: app.currentTimestamp
        font.pixelSize: Theme.fontSizeSmall
        font.family: Theme.fontFamilyHeading
        color: "#FFD25A"
        style: Text.Outline
        styleColor: Qt.rgba(0, 0, 0, 0.85)
    }

    /* ------------------------------------------------------------------ */
    /* Left column: settings + LED brightness                              */
    /* ------------------------------------------------------------------ */
    Item {
        id: leftBar
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        /* Same width as the right-hand column. It has to hold a roll dial big
         * enough to grab and drag around, which the previous narrow strip was
         * not — and a matching width also puts the picture back in the optical
         * centre of the screen. */
        width: Theme.itemSizeLarge + 2 * Theme.paddingLarge

        /* Mirror of the right column's fade: vertical gradient in a rectangle
         * with width and height swapped, rotated +90° so the dark end lands on
         * the left. Gradient.orientation is Qt 5.12+ and Sailfish is on 5.6 —
         * using it makes the whole page fail to load. */
        Rectangle {
            anchors.centerIn: parent
            width: parent.height
            height: parent.width
            rotation: 90
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.65) }
            }
        }

        /* Gear in the corner, with the same gap to the top edge as to the left
         * edge — a corner control should look like it sits in the corner. */
        IconButton {
            id: settingsButton
            anchors {
                left: parent.left
                top: parent.top
                leftMargin: Theme.paddingMedium
                topMargin: Theme.paddingMedium
            }
            icon.source: "image://theme/icon-m-developer-mode"
            onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
        }

        GainSlider {
            id: gainSlider
            anchors {
                /* Sits noticeably below the gear rather than right under it,
                 * and stops short of the roll dial at the bottom. */
                top: settingsButton.bottom
                topMargin: Theme.itemSizeSmall
                bottom: rollIndicator.top
                bottomMargin: Theme.paddingLarge
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width
            maxGain: app.camera.maxGain
            gain: app.camera.gain
            onGainChanged: {
                app.camera.gain = gain
                app.settings.gain = gain
            }
        }

        RollIndicator {
            id: rollIndicator
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
            width: Theme.itemSizeLarge
            height: width
            roll: viewfinder.roll
            /* Drag the ring to turn the picture, tap the middle to level it. */
            onRollRequested: viewfinder.roll = degrees
            onResetRequested: viewfinder.resetRoll()
        }
    }

    /* ------------------------------------------------------------------ */
    /* Right column: photo, video, gallery                                 */
    /* ------------------------------------------------------------------ */
    Item {
        id: controlBar
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        width: Theme.itemSizeLarge + 2 * Theme.paddingLarge

        Rectangle {
            anchors.centerIn: parent
            width: parent.height
            height: parent.width
            rotation: -90
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.65) }
            }
        }

        ShutterButton {
            id: shutter
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
                topMargin: Theme.paddingLarge
            }
            enabled: app.camera.streaming
            onClicked: {
                if (app.takeSnapshot())
                    flash.flash()
            }
        }

        RecordButton {
            id: recordButton
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: shutter.bottom
                /* Deliberately generous: these two do opposite things and get
                 * hit without looking. */
                topMargin: Theme.itemSizeSmall
            }
            recording: app.recorder.recording
            enabled: app.camera.streaming || app.recorder.recording
            onClicked: app.toggleRecording()
        }

        /* --- last capture, doubles as the way into the gallery --- */
        BackgroundItem {
            id: galleryButton
            width: Theme.itemSizeMedium
            height: width
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
            onClicked: pageStack.push(Qt.resolvedUrl("GalleryPage.qml"))

            Rectangle {
                anchors.fill: parent
                radius: Theme.paddingSmall
                color: "transparent"
                border.width: 1
                border.color: Theme.rgba(Theme.lightPrimaryColor, 0.4)
                clip: true

                Image {
                    id: lastThumb
                    anchors.fill: parent
                    anchors.margins: 2
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    /* Videos have no decodable thumbnail here, so fall back to
                     * the icon rather than showing a broken image. */
                    source: app.captures.lastCapturePath !== "" &&
                            app.captures.lastCapturePath.indexOf(".mp4") < 0
                            ? "file://" + app.captures.lastCapturePath : ""
                    visible: status === Image.Ready
                }
                Image {
                    anchors.centerIn: parent
                    visible: !lastThumb.visible
                    source: "image://theme/icon-m-image"
                    opacity: app.captures.count > 0 ? 0.9 : 0.35
                }
            }

            Rectangle {
                visible: app.captures.count > 0
                anchors { right: parent.right; top: parent.top }
                width: countLabel.width + Theme.paddingSmall
                height: countLabel.height
                radius: Theme.paddingSmall
                color: Theme.highlightBackgroundColor
                Label {
                    id: countLabel
                    anchors.centerIn: parent
                    text: app.captures.count
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.primaryColor
                }
            }
        }
    }

    function formatDuration(ms) {
        var total = Math.floor(ms / 1000)
        var m = Math.floor(total / 60)
        var s = total % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    /* ------------------------------------------------------------------ */
    /* Shutter flash + error feedback                                      */
    /* ------------------------------------------------------------------ */
    Rectangle {
        id: flash
        anchors.fill: parent
        color: "white"
        opacity: 0
        function flash() { flashAnim.restart() }
        SequentialAnimation {
            id: flashAnim
            NumberAnimation { target: flash; property: "opacity"; to: 0.75; duration: 60 }
            NumberAnimation { target: flash; property: "opacity"; to: 0.0;  duration: 220 }
        }
    }

    Connections {
        target: app.captures
        onError: errorBanner.show(message)
    }
    Connections {
        target: app.recorder
        onLastErrorChanged: {
            if (app.recorder.lastError !== "")
                errorBanner.show(app.recorder.lastError)
        }
    }

    Rectangle {
        id: errorBanner
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: errorLabel.height + 2 * Theme.paddingMedium
        color: Theme.rgba(Theme.errorColor, 0.9)
        opacity: 0
        visible: opacity > 0

        function show(message) {
            errorLabel.text = message
            errorAnim.restart()
        }

        Label {
            id: errorLabel
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left; right: parent.right
                leftMargin: Theme.horizontalPageMargin
                rightMargin: Theme.horizontalPageMargin
            }
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.lightPrimaryColor
        }

        SequentialAnimation {
            id: errorAnim
            NumberAnimation { target: errorBanner; property: "opacity"; to: 1.0; duration: 150 }
            PauseAnimation { duration: 4000 }
            NumberAnimation { target: errorBanner; property: "opacity"; to: 0.0; duration: 400 }
        }
    }
}
