import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    objectName: "aboutDialog"
    required property var applicationInfo

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.max(0, Math.min(560,
                                (parent ? parent.width : 592) - Theme.spaceXl))
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
        spacing: Theme.spaceLg

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            SZHBrandLogo {
                iconOnly: true
                darkVariant: Theme.darkMode
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2xs

                Label {
                    Layout.fillWidth: true
                    text: qsTr("SZHBooks")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.displayFontSize
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Desktop reader")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyFontSize
                }
            }

            SZHIconButton {
                id: closeButton

                objectName: "closeAboutButton"
                symbol: "\u00d7"
                toolTip: qsTr("Close")
                onClicked: root.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.borderColor
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spaceLg
            rowSpacing: Theme.spaceSm

            Label {
                text: qsTr("Version")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            Label {
                objectName: "aboutVersionLabel"
                Layout.fillWidth: true
                text: root.applicationInfo.applicationVersion
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                font.weight: Font.Medium
            }

            Label {
                text: qsTr("Runtime")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            Label {
                Layout.fillWidth: true
                text: "Qt " + root.applicationInfo.qtVersion
                      + " | " + root.applicationInfo.systemArchitecture
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                elide: Text.ElideRight
            }

            Label {
                text: qsTr("Storage")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            Label {
                Layout.fillWidth: true
                text: root.applicationInfo.portableMode
                      ? qsTr("Portable") : qsTr("Installed")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
            }

            Label {
                text: qsTr("Profile")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            Label {
                Layout.fillWidth: true
                text: root.applicationInfo.profileDirectoryPath
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                elide: Text.ElideMiddle
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.applicationInfo.copyrightNotice
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            SZHButton {
                objectName: "openProfileFolderButton"
                text: qsTr("Profile folder")
                symbol: "\u25a1"
                variant: "secondary"
                onClicked: root.applicationInfo.openProfileDirectory()
            }

            SZHButton {
                objectName: "openNoticesButton"
                text: qsTr("Licenses")
                symbol: "i"
                variant: "secondary"
                enabled: root.applicationInfo.thirdPartyNoticesAvailable
                onClicked: root.applicationInfo.openThirdPartyNotices()
            }

            Item {
                Layout.fillWidth: true
            }

            SZHButton {
                text: qsTr("Close")
                onClicked: root.close()
            }
        }
    }

    onOpened: closeButton.forceActiveFocus()
}
