/*
 * RecordButton.qml — start / stop video recording.
 *
 * Deliberately the SAME SIZE as ShutterButton. These two are the controls that
 * get pressed without looking, and a size difference is the kind of thing you
 * only notice after you have started a recording when you wanted a photo. They
 * are distinguished by shape and colour instead: a red disc that becomes a red
 * square while recording — the universal record/stop affordance.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: root

    /* Same footprint as ShutterButton. */
    width: Theme.itemSizeLarge
    height: width

    property bool recording: false
    property bool pressedDown: pressed && containsMouse

    Rectangle {
        id: outerRing
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 4
        border.color: root.enabled ? Theme.rgba(Theme.lightPrimaryColor, 0.95)
                                   : Theme.rgba(Theme.lightSecondaryColor, 0.3)
    }

    Rectangle {
        id: inner
        anchors.centerIn: parent
        width: parent.width * (root.recording ? 0.46
                                              : (root.pressedDown ? 0.62 : 0.72))
        height: width
        radius: root.recording ? Theme.paddingSmall / 2 : width / 2
        color: root.enabled ? "#E4382E" : Theme.rgba("#E4382E", 0.3)

        Behavior on width  { NumberAnimation { duration: 140 } }
        Behavior on radius { NumberAnimation { duration: 140 } }
    }
}
