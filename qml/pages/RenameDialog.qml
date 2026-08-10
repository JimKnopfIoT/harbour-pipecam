/*
 * RenameDialog.qml — give a capture a name that means something.
 *
 * The default filename is a timestamp, which is right for sorting and useless
 * for remembering. On a real job you want "Haus12-Keller-Riss" so the file is
 * still identifiable a week later in a report.
 *
 * Only the base name is editable. The extension is not the user's to change:
 * it describes what the file actually contains, and renaming a .mp4 to .jpg
 * would break the gallery and every other app that opens it. It is shown next
 * to the field so it is obvious it is being kept.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    allowedOrientations: Orientation.Landscape

    /* Set by the caller. `baseName` is read back after acceptance. */
    property int index: -1
    property string baseName: ""
    property string suffix: ""

    /* An empty or whitespace-only name would either fail in the backend or
     * produce a file called ".jpg"; refuse it here instead. */
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
            /* No auto-capitalisation or predictive text: these names are codes
             * and abbreviations, and correction fights the user. */
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: if (dialog.canAccept) dialog.accept()

            Component.onCompleted: {
                forceActiveFocus()
                /* Select everything so typing replaces the timestamp outright,
                 * which is what someone renaming a default name wants. */
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
