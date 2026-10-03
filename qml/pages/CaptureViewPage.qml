/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
import QtQuick 2.6
import Sailfish.Silica 1.0
import QtMultimedia 5.6
import Sailfish.Share 1.0

Page {
    id: page

    /* Set by GalleryPage: named roles resolve only in delegates. */
    property int index: -1
    property bool isVideo: false
    property string filePath: ""
    property string fileUrl: ""
    property string fileName: ""
    property string sizeText: ""

    allowedOrientations: Orientation.Landscape

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: parent.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete")
                onClicked: remorse.execute(qsTr("Deleting"), function() {
                    if (app.captures.remove(page.index))
                        pageStack.pop()
                })
            }
            MenuItem {
                text: qsTr("Rename")
                onClicked: {
                    var dialog = pageStack.push(
                        Qt.resolvedUrl("RenameDialog.qml"),
                        { index: page.index,
                          baseName: app.captures.baseName(page.index),
                          suffix: page.isVideo ? ".mp4" : ".jpg" })
                    dialog.accepted.connect(function() {
                        if (app.captures.rename(page.index, dialog.baseName)) {
                            /* Pushed copy; does not track the model. */
                            page.fileName = dialog.baseName + dialog.suffix
                            page.filePath = page.filePath.replace(/[^\/]+$/,
                                                                  page.fileName)
                            page.fileUrl = "file://" + page.filePath
                        }
                    })
                }
            }
            MenuItem {
                text: qsTr("Share")
                onClicked: shareAction.trigger()
            }
        }

        RemorsePopup { id: remorse }

        Image {
            id: photo
            visible: !page.isVideo
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            source: page.isVideo ? "" : page.fileUrl
            smooth: true
        }

        Video {
            id: player
            visible: page.isVideo
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
            source: page.isVideo ? page.fileUrl : ""
            autoPlay: false

            MouseArea {
                anchors.fill: parent
                onClicked: player.playbackState === MediaPlayer.PlayingState
                           ? player.pause() : player.play()
            }
        }

        Image {
            anchors.centerIn: parent
            visible: page.isVideo && player.playbackState !== MediaPlayer.PlayingState
            source: "image://theme/icon-l-play"
            opacity: 0.85
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: caption.height + 2 * Theme.paddingMedium
            color: Qt.rgba(0, 0, 0, 0.55)

            Column {
                id: caption
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left; right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.horizontalPageMargin
                }
                Label {
                    width: parent.width
                    text: page.fileName
                    truncationMode: TruncationMode.Fade
                    color: Theme.lightPrimaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }
                Label {
                    text: page.sizeText
                    color: Theme.lightSecondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }
        }
    }

    ShareAction {
        id: shareAction
        resources: [ page.filePath ]
        mimeType: page.isVideo ? "video/mp4" : "image/jpeg"
    }
}
