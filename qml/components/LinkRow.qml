/*
 * LinkRow.qml — a tappable link: label above, URL below, opens in the browser.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: row
    property string label
    property string url

    width: parent ? parent.width : 0
    height: linkCol.height + 2 * Theme.paddingSmall
    onClicked: Qt.openUrlExternally(url)

    Column {
        id: linkCol
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter

        Label {
            text: row.label
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
        Label {
            width: parent.width
            text: row.url
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.highlightColor
            truncationMode: TruncationMode.Fade
        }
    }
}
