/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Landscape

    /* Reload on activation: cover action or recorder may add files meanwhile. */
    onStatusChanged: if (status === PageStatus.Activating) app.captures.refresh()

    /* Page-level: must outlive the delegate. */
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

            /* Roles passed explicitly, see CaptureViewPage. */
            onClicked: pageStack.push(Qt.resolvedUrl("CaptureViewPage.qml"), {
                index: model.index,
                isVideo: model.isVideo,
                filePath: model.path,
                fileUrl: model.url,
                fileName: model.fileName,
                sizeText: model.sizeText
            })

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
                    /* RemorsePopup on the page: a RemorseItem in the delegate dies with the row. */
                    onClicked: {
                        /* Capture the row now: model.index shifts before this runs. */
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
                    source: model.isVideo ? "" : model.url
                    sourceSize.width: grid.cellWidth
                }

                Image {
                    anchors.centerIn: parent
                    visible: model.isVideo
                    source: "image://theme/icon-m-video"
                    opacity: 0.8
                }
            }

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
