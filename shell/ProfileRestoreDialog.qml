import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    objectName: "profileRestoreDialog"

    property url backupUrl

    signal restoreConfirmed

    readonly property string backupName: {
        const parts = decodeURIComponent(root.backupUrl.toString()).split("/")
        return parts.length > 0 ? parts[parts.length - 1] : ""
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.max(0, Math.min(440,
                                (parent ? parent.width : 472) - Theme.spaceXl))
    padding: Theme.spaceLg
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: Theme.darkMode ? "#99000000" : "#66000000"
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusLg
        border.color: Theme.borderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Label {
            Layout.fillWidth: true
            text: qsTr("Restore local profile?")
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.titleFontSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            visible: root.backupName.length > 0
            text: root.backupName
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyFontSize
            elide: Text.ElideMiddle
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("This replaces current settings, library, reading positions, bookmarks and notes. Your book files are not changed.")
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyFontSize
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Item {
                Layout.fillWidth: true
            }

            SZHButton {
                id: cancelButton

                objectName: "cancelRestoreButton"
                text: qsTr("Cancel")
                variant: "secondary"
                onClicked: root.close()
            }

            SZHButton {
                objectName: "confirmRestoreButton"
                text: qsTr("Restore")
                onClicked: {
                    root.restoreConfirmed()
                    root.close()
                }
            }
        }
    }

    onOpened: cancelButton.forceActiveFocus()
}
