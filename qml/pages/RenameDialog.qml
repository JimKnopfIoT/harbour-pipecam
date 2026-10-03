/* Copyright (C) 2026  JimKnopfIoT — GPLv3 or later. */
/* Base name only; extension is fixed. */
import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    allowedOrientations: Orientation.Landscape

    /* `baseName` is read back after acceptance. */
    property int index: -1
    property string baseName: ""
    property string suffix: ""

    canAccept: nameField.text.trim() !== ""

    onAccepted: dialog.baseName = nameField.text.trim()

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Rename")
            cancelText: qsTr("Cancel")
        }

        TextField {
            id: nameField
            width: parent.width
            label: qsTr("Name")
            placeholderText: qsTr("Name")
            text: dialog.baseName
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: if (dialog.canAccept) dialog.accept()

            Component.onCompleted: {
                forceActiveFocus()
                selectAll()
            }
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
            text: qsTr("Saved as “%1”").arg(nameField.text.trim() + dialog.suffix)
        }
    }
}
