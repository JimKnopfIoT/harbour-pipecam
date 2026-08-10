/*
 * harbour-pipecam.qml — application root.
 *
 * Owns the four backend objects and hands them to the pages, rather than
 * letting each page create its own. The camera in particular MUST be a
 * singleton: it holds an exclusive USB claim on the device, so a second
 * instance would fail to open it and the first would appear to freeze.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Nemo.KeepAlive 1.2
import harbour.pipecam 1.0

import "pages"
import "cover"

ApplicationWindow {
    id: app

    /* ---- backend --------------------------------------------------------- */
    property alias camera: cameraBackend
    property alias recorder: videoRecorder
    property alias captures: captureStore

    /* ---- persisted settings ---------------------------------------------- */
    /* dconf under /apps/harbour-pipecam/ — writable without any extra
     * permission, and survives reinstalls. */
    property alias settings: settingsGroup

    PipeCamera {
        id: cameraBackend

        /* The push-button on the camera cable. Ten metres down a pipe, the
         * phone is not in your hand — this is the only control you can reach,
         * so it is bound to whatever the user says is most useful.
         *
         * "off" is a real setting, not padding: the button sits right next to
         * the dimmer wheel on the cable, so it is easy to catch by accident
         * while adjusting the light, and an accidental snapshot mid-inspection
         * is clutter you have to sort out afterwards. */
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
        /* Frames are pulled straight from the camera in C++. They deliberately
         * never pass through QML: a QByteArray crossing the QML boundary is
         * converted to a JavaScript string, which would corrupt every JPEG
         * byte above 0x7F. */
        source: cameraBackend
        /* Burnt into every recorded frame while the setting is on. Empty means
         * the recorder takes its lossless path and muxes the camera's own JPEGs
         * untouched. Bound rather than set once, so the clock keeps ticking
         * inside a long recording. */
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
        property string cableButtonAction: "snapshot" /* or "record" */
        property bool showGrid: false
        /* Burn the capture date/time into the corner of the picture. Off by
         * default because it costs a re-encode on every snapshot — see
         * CaptureStore::saveSnapshot(). */
        property bool showTimestamp: false
        /* Software brightness, 1.0 = untouched. Replaces what started out as an
         * LED dimmer: the camera's lamps provably cannot be driven from the
         * phone (see the “Protocol” section of README.md), so the picture gets brightened
         * instead. */
        property real gain: 1.0
        /* Apply the viewfinder's roll to what is saved, not just to what is
         * shown. On by default: having straightened the picture on screen,
         * finding the recording still sideways is a surprise, not a feature. */
        property bool captureRotated: true
    }

    /* The viewfinder's current roll, lifted to app level so the recorder and the
     * snapshot path can see it. It lives in the view — that is where the gesture
     * is — but two things outside the view need to know about it. Not persisted:
     * the head's twist belongs to one run down one pipe. */
    property real viewRoll: 0
    readonly property real captureRoll: settings.captureRotated ? viewRoll : 0

    /* One clock for the whole app, so the label on screen and the text burnt
     * into a snapshot can never disagree. Ticks once a second — the stamp has
     * second resolution, so anything faster is wasted wake-ups. */
    property string currentTimestamp: ""
    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: app.currentTimestamp =
            Qt.formatDateTime(new Date(), "yyyy-MM-dd  hh:mm:ss")
    }

    /* Inspecting a pipe means staring at the screen without touching it. The
     * default display timeout blanks after 30 s, which is unusable here — so
     * hold blanking off while a frame is actually arriving, and only then (a
     * stopped camera should not keep the screen alive in someone's pocket). */
    DisplayBlanking {
        preventBlanking: settings.keepDisplayOn && cameraBackend.streaming
    }

    /* ---- shared actions, so cover and viewfinder do exactly the same thing - */

    function takeSnapshot() {
        if (!cameraBackend.streaming)
            return false
        /* Pass the camera, not its bytes — see the VideoRecorder note above.
         * The second argument is the text to burn in; empty means "write the
         * camera's JPEG untouched", which is the lossless fast path. */
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

    /* NOTE: there is deliberately no Connections block feeding frames to the
     * recorder from here. It pulls them itself in C++ via its `source` property.
     * An earlier version did push from QML, and it was quietly broken twice
     * over: currentJpeg is not a QML-visible property, so the argument was
     * always `undefined`, and had it worked, QML would have converted the
     * QByteArray to a JavaScript string and corrupted every JPEG. Frames must
     * not cross the QML boundary. */

    /* A recording is a file being written; losing the app would truncate it.
     * Close it cleanly on shutdown so the .mp4 always has its moov atom. */
    Component.onDestruction: {
        if (videoRecorder.recording)
            videoRecorder.stop()
        cameraBackend.stop()
    }

    Component.onCompleted: {
        /* Restore the brightness before starting, so the first frame already
         * looks the way the user left it rather than flashing dark first. */
        cameraBackend.gain = settings.gain
        /* Always on. There was a setting for this; it was pointless — opening a
         * camera app and finding no picture is never what anyone wants, and the
         * camera can still be stopped from the settings page if it is ever in
         * the way. */
        cameraBackend.start()
    }

    initialPage: Component { ViewfinderPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    /* Landscape only, everywhere, with no auto-rotation.
     *
     * The camera sensor is 4:3 and the job is watching a picture, so landscape
     * is simply the right shape — a portrait phone wastes two thirds of the
     * screen on black bars. More importantly, this work happens with the phone
     * propped up or held at odd angles while both hands are busy with ten metres
     * of cable, and an accelerometer-driven rotation at that moment is actively
     * disruptive. Orientation.Landscape (not LandscapeMask) means it will not
     * even flip 180 degrees on its own.
     *
     * Every page sets this for itself as well — a Page's own allowedOrientations
     * wins over the window's, so relying on the window alone would silently let
     * a future page rotate. */
    allowedOrientations: Orientation.Landscape
}
