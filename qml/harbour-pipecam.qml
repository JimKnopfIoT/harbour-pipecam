/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
/* Backends are singletons: PipeCamera holds an exclusive USB claim. */
import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Nemo.KeepAlive 1.2
import harbour.pipecam 1.0

import "pages"
import "cover"

ApplicationWindow {
    id: app

    property alias camera: cameraBackend
    property alias recorder: videoRecorder
    property alias captures: captureStore

    property alias settings: settingsGroup

    PipeCamera {
        id: cameraBackend

        onButtonClicked: {
            if (settings.cableButtonAction === "off")
                return
            if (settings.cableButtonAction === "record") {
                app.toggleRecording()
            } else {
                app.takeSnapshot()
            }
        }
    }

    VideoRecorder {
        id: videoRecorder
        /* Frames stay in C++: a QByteArray in QML becomes a JS string (corrupts bytes > 0x7F). */
        source: cameraBackend
        /* Empty: lossless path, camera JPEGs muxed untouched. */
        overlayText: settings.showTimestamp ? app.currentTimestamp : ""
        rotation: app.captureRoll
        onRecordingFinished: captureStore.registerCapture(path)
    }

    CaptureStore {
        id: captureStore
    }

    ConfigurationGroup {
        id: settingsGroup
        path: "/apps/harbour-pipecam"

        property bool mirrored: false
        property int fillMode: 0                     /* VideoFrameItem.PreserveAspectFit */
        property bool keepDisplayOn: true
        property string cableButtonAction: "snapshot" /* "snapshot" | "record" | "off" */
        property bool showGrid: false
        /* Non-empty stamp forces a re-encode per snapshot. */
        property bool showTimestamp: false
        /* Software gain, 1.0 = untouched. */
        property real gain: 1.0
        property bool captureRotated: true
        /* Persisted: startup problems need it before launch. */
        property bool verboseLog: false
    }

    Binding {
        target: diagLog
        property: "verbose"
        value: settingsGroup.verboseLog
    }

    property real viewRoll: 0
    readonly property real captureRoll: settings.captureRotated ? viewRoll : 0

    property string currentTimestamp: ""
    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: app.currentTimestamp =
            Qt.formatDateTime(new Date(), "yyyy-MM-dd  hh:mm:ss")
    }

    DisplayBlanking {
        preventBlanking: settings.keepDisplayOn && cameraBackend.streaming
    }

    function takeSnapshot() {
        if (!cameraBackend.streaming)
            return false
        /* Pass the camera, not its bytes; empty stamp = lossless path. */
        var stamp = settings.showTimestamp ? app.currentTimestamp : ""
        return captureStore.saveSnapshot(cameraBackend, stamp, app.captureRoll) !== ""
    }

    function toggleRecording() {
        if (videoRecorder.recording) {
            videoRecorder.stop()
            return false
        }
        if (!cameraBackend.streaming)
            return false
        var path = captureStore.newVideoPath()
        if (path === "")
            return false
        return videoRecorder.start(path, cameraBackend.frameWidth, cameraBackend.frameHeight)
    }

    /* stop() finalizes the .mp4 (moov atom). */
    Component.onDestruction: {
        if (videoRecorder.recording)
            videoRecorder.stop()
        cameraBackend.stop()
    }

    Component.onCompleted: {
        cameraBackend.gain = settings.gain
        cameraBackend.start()
    }

    initialPage: Component { ViewfinderPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    /* Pages set this too: a Page's allowedOrientations overrides the window's. */
    allowedOrientations: Orientation.Landscape
}
