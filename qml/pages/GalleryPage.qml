/*
 * GalleryPage.qml — everything captured, newest first.
 *
 * A grid rather than a list: these are pictures, and at 640x480 a thumbnail is
 * already most of the information. Videos are marked with a play badge and
 * their duration is not shown — reading it would mean demuxing every file just
 * to draw a list.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Landscape

    /* Files can appear from the cover action or a finished recording while this
     * page sits in the stack; re-read the folders whenever it comes forward. */
    onStatusChanged: if (status === PageStatus.Activating) app.captures.refresh()

    /* Page-level, so it survives the delegate it was triggered from. */
    RemorsePopup { id: remorse }

    SilicaGridView {
        id: grid
        anchors.fill: parent
        cellWidth: Math.floor(page.width / (page.isPortrait ? 3 : 5))
        cellHeight: cellWidth

        header: PageHeader {
            title: qsTr("Captures")
            description: app.captures.count > 0
                         ? qsTr("%n item(s)", "", app.captures.count) : ""
        }

        model: app.captures

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: app.captures.refresh()
            }
        }

        delegate: BackgroundItem {
            id: cell
            width: grid.cellWidth
            height: grid.cellHeight

            /* Hand the delegate's role values over directly — see the comment
             * at the top of CaptureViewPage for why it does not read them back
             * out of the model itself. */
            onClicked: pageStack.push(Qt.resolvedUrl("CaptureViewPage.qml"), {
                index: model.index,
                isVideo: model.isVideo,
                filePath: model.path,
                fileUrl: model.url,
                fileName: model.fileName,
                sizeText: model.sizeText
            })

            /* Press and hold for rename/delete — the Silica idiom for a grid,
             * and the only one that does not steal a tap from "open it". */
            onPressAndHold: contextMenu.open(cell)

            ContextMenu {
                id: contextMenu

                MenuItem {
                    text: qsTr("Rename")
                    onClicked: {
                        var dialog = pageStack.push(
                            Qt.resolvedUrl("RenameDialog.qml"),
                            { index: model.index,
                              baseName: app.captures.baseName(model.index),
                              suffix: model.isVideo ? ".mp4" : ".jpg" })
                        dialog.accepted.connect(function() {
                            app.captures.rename(dialog.index, dialog.baseName)
                        })
                    }
                }

                MenuItem {
                    text: qsTr("Delete")
                    /* A countdown rather than an instant delete: these are the
                     * only record of something that has since been closed up
                     * again, and a mis-tap in a grid is easy.
                     *
                     * The countdown lives on the PAGE (RemorsePopup), not in
                     * the delegate. A RemorseItem inside the delegate would be
                     * destroyed together with the delegate the moment the row
                     * is removed — and `remorseAction()` is not a method a
                     * delegate has at all, so calling it just threw silently
                     * and the file was never deleted. */
                    onClicked: {
                        /* Capture the row NOW: `model.index` shifts as soon as
                         * anything else is added or removed, and this callback
                         * runs seconds later. */
                        var row = model.index
                        remorse.execute(qsTr("Deleting"), function() {
                            app.captures.remove(row)
                        })
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.15)
                clip: true

                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    /* Only photos can be shown directly. Videos get the badge
                     * below on an empty tile — cheap and honest. */
                    source: model.isVideo ? "" : model.url
                    /* 640x480 source into a ~250px cell: ask the loader for the
                     * smaller size so we do not keep full frames in memory. */
                    sourceSize.width: grid.cellWidth
                }

                Image {
                    anchors.centerIn: parent
                    visible: model.isVideo
                    source: "image://theme/icon-m-video"
                    opacity: 0.8
                }
            }

            /* Video badge, bottom-left, over whatever is behind it. */
            Rectangle {
                visible: model.isVideo
                anchors {
                    left: parent.left; bottom: parent.bottom
                    margins: Theme.paddingSmall
                }
                width: badge.width + Theme.paddingSmall
                height: badge.height
                radius: 2
                color: Qt.rgba(0, 0, 0, 0.6)
                Label {
                    id: badge
                    anchors.centerIn: parent
                    text: "MP4"
                    font.pixelSize: Theme.fontSizeTiny
                    color: Theme.lightPrimaryColor
                }
            }
        }

        ViewPlaceholder {
            enabled: app.captures.count === 0
            text: qsTr("No captures yet")
            hintText: qsTr("Snapshots and recordings you make will appear here.")
        }

        VerticalScrollDecorator {}
    }
}
