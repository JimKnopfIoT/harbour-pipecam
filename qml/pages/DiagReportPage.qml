/*
 * DiagReportPage.qml — build, read, copy and save the diagnostic report.
 *
 * The report itself is assembled in C++ (src/diag/diagreport.h) and arrives
 * here already anonymised. This page only collects what QML alone knows — the
 * camera's state and the settings — and decides two things with the user:
 *
 *   * whether to run the claim test, which needs the live picture stopped for
 *     a moment so that the app's own hold on the camera is out of the way;
 *   * whether to use the root helper, which is a deliberate step behind a
 *     dialog that says what it reads.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.pipecam 1.0

Page {
    id: page
    allowedOrientations: Orientation.Landscape

    property string report: ""
    property bool busy: false
    property bool useRoot: false
    property string savedPath: ""

    DiagReport { id: diagReport }

    /* Everything the report cannot read for itself. Flat keys, plain values:
     * the C++ side prints them as they come. Taken BEFORE the claim test stops
     * the camera, or every report would say "Off". */
    function appState() {
        var s = app.settings
        var info = app.camera.deviceInfo || {}
        return {
            "camera.status": app.camera.statusText,
            "camera.detail": app.camera.statusDetail,
            "camera.running": app.camera.running,
            "camera.frames": app.camera.frameCount,
            "camera.fps": app.camera.fps.toFixed(1),
            "camera.product": info.product !== undefined ? info.product : "",
            "camera.manufacturer": info.manufacturer !== undefined ? info.manufacturer : "",
            "camera.serial": info.serial !== undefined ? info.serial : "",
            "recorder.recording": app.recorder.recording,
            "screen": Screen.width + "x" + Screen.height,
            "settings.mirrored": s.mirrored,
            "settings.fillMode": s.fillMode,
            "settings.keepDisplayOn": s.keepDisplayOn,
            "settings.cableButtonAction": s.cableButtonAction,
            "settings.showGrid": s.showGrid,
            "settings.showTimestamp": s.showTimestamp,
            "settings.gain": s.gain,
            "settings.captureRotated": s.captureRotated,
            "settings.verboseLog": s.verboseLog
        }
    }

    function generate() {
        busy = true
        report = ""
        savedPath = ""
        /* Let the busy indicator paint before the blocking work starts. */
        buildTimer.restart()
    }

    Timer {
        id: buildTimer
        interval: 50
        onTriggered: {
            var state = page.appState()
            var claimTest = claimSwitch.checked && !app.recorder.recording
            var wasRunning = app.camera.running
            if (claimTest && wasRunning)
                app.camera.stop()
            page.report = diagReport.build(state, claimTest, page.useRoot)
            if (claimTest && wasRunning)
                app.camera.start()
            page.busy = false
        }
    }

    function enableRoot() {
        var dlg = pageStack.push(Qt.resolvedUrl("RootConfirmDialog.qml"))
        dlg.accepted.connect(function() {
            page.useRoot = true
            rootHelper.setHelper(true)
        })
    }

    Component.onDestruction: {
        if (useRoot)
            rootHelper.setHelper(false)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingSmall

            PageHeader {
                title: qsTr("Diagnostic report")
                description: qsTr("Anonymised, ready to post")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Collects app and system versions, the camera's "
                           + "complete USB description and the app's log. "
                           + "Serial numbers, host name, user name, MAC and IP "
                           + "addresses are removed on the phone, before you "
                           + "see the text.")
            }

            TextSwitch {
                id: claimSwitch
                checked: true
                enabled: !app.recorder.recording
                text: qsTr("Test taking over the camera")
                description: qsTr("Stops the live picture for a moment and tries "
                                  + "to claim the camera's USB interfaces, to see "
                                  + "whether something else is holding them.")
            }

            TextSwitch {
                text: qsTr("Include root data")
                description: qsTr("Starts a read-only helper as root for the "
                                  + "kernel's view: which process holds the "
                                  + "camera, kernel log and journal lines about "
                                  + "USB. Stops again when you leave this page.")
                checked: page.useRoot
                automaticCheck: false
                onClicked: {
                    if (page.useRoot) {
                        page.useRoot = false
                        rootHelper.setHelper(false)
                    } else {
                        page.enableRoot()
                    }
                }
            }

            Label {
                visible: page.useRoot
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: rootHelper.active ? Theme.highlightColor : Theme.secondaryColor
                text: rootHelper.active ? qsTr("Helper connected.")
                    : rootHelper.lastError !== "" ? qsTr("Helper could not be started: %1").arg(rootHelper.lastError)
                    : qsTr("Starting helper…")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: page.report === "" ? qsTr("Create report") : qsTr("Create again")
                enabled: !page.busy && (!page.useRoot || rootHelper.active
                                        || rootHelper.lastError !== "")
                onClicked: page.generate()
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Medium
                running: page.busy
                visible: running
            }

            Item {
                visible: page.report !== ""
                width: parent.width
                height: actions.height + Theme.paddingMedium

                Row {
                    id: actions
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingLarge

                    Button {
                        text: qsTr("Copy")
                        onClicked: Clipboard.text = page.report
                    }
                    Button {
                        text: qsTr("Save")
                        onClicked: page.savedPath = diagReport.save(page.report)
                    }
                    Button {
                        text: qsTr("Open issues")
                        onClicked: Qt.openUrlExternally(
                            "https://github.com/JimKnopfIoT/harbour-pipecam/issues")
                    }
                }
            }

            Label {
                visible: page.savedPath !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.highlightColor
                text: qsTr("Saved to %1").arg(page.savedPath)
            }

            Label {
                visible: page.report !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Please read it through once before posting. Paste "
                           + "it into a GitHub issue as it is — it is already "
                           + "formatted for that.")
            }

            Label {
                visible: page.report !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WrapAnywhere
                font.family: "monospace"
                font.pixelSize: Theme.fontSizeTiny
                color: Theme.primaryColor
                textFormat: Text.PlainText
                text: page.report
            }
        }

        VerticalScrollDecorator {}
    }
}
