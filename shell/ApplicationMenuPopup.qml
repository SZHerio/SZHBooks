import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var diagnosticService

    signal backupRequested
    signal restoreRequested
    signal aboutRequested

    width: 248
    padding: Theme.space2xs
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: Theme.strongBorderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        SZHButton {
            id: backupButton

            Layout.fillWidth: true
            text: qsTr("Back up profile")
            symbol: "\u2191"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: {
                root.close()
                root.backupRequested()
            }
        }

        SZHButton {
            Layout.fillWidth: true
            text: qsTr("Restore profile")
            symbol: "\u2193"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: {
                root.close()
                root.restoreRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXs
            Layout.rightMargin: Theme.spaceXs
            Layout.topMargin: Theme.space2xs
            Layout.bottomMargin: Theme.space2xs
            implicitHeight: 1
            color: Theme.borderColor
        }

        SZHButton {
            Layout.fillWidth: true
            text: qsTr("Diagnostics folder")
            symbol: "\u25a1"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: {
                root.close()
                root.diagnosticService.openLogDirectory()
            }
        }

        SZHButton {
            Layout.fillWidth: true
            text: qsTr("About SZHBooks")
            symbol: "i"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: {
                root.close()
                root.aboutRequested()
            }
        }
    }

    onOpened: backupButton.forceActiveFocus()
}
