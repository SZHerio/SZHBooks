import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var fileService
    property string collectionPath: ""
    property bool confirmRemoval: false

    signal collectionRenamed(string collectionPath)
    signal collectionRemoved

    function openFor(path) {
        root.collectionPath = path
        root.confirmRemoval = false
        const parts = path.split("/")
        renameField.text = parts.length > 0 ? parts[parts.length - 1] : ""
        root.open()
    }

    function renameFolder() {
        const renamedPath = root.fileService.renameCollection(root.collectionPath, renameField.text)
        if (renamedPath.length === 0)
            return
        root.collectionRenamed(renamedPath)
        root.close()
    }

    function removeFolder() {
        if (!root.fileService.removeCollection(root.collectionPath))
            return
        root.collectionRemoved()
        root.close()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.max(0, Math.min(440, (parent ? parent.width : 472) - Theme.spaceXl))
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
            spacing: Theme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2xs

                Label {
                    text: qsTr("Folder actions")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.titleFontSize
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.collectionPath
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                    elide: Text.ElideMiddle
                }
            }

            SZHIconButton {
                symbol: "\u00d7"
                toolTip: qsTr("Close")
                onClicked: root.close()
            }
        }

        ColumnLayout {
            visible: !root.confirmRemoval
            Layout.fillWidth: true
            spacing: Theme.spaceLg

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Label {
                    text: qsTr("Folder name")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    SZHTextField {
                        id: renameField

                        objectName: "collectionRenameField"
                        Layout.fillWidth: true
                        Keys.onReturnPressed: root.renameFolder()
                        Keys.onEnterPressed: root.renameFolder()
                    }

                    SZHButton {
                        objectName: "renameCollectionButton"
                        text: qsTr("Rename")
                        variant: "secondary"
                        enabled: renameField.text.trim().length > 0
                        onClicked: root.renameFolder()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.borderColor
            }

            SZHButton {
                objectName: "removeCollectionButton"
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Remove empty folder")
                variant: "secondary"
                onClicked: root.confirmRemoval = true
            }
        }

        ColumnLayout {
            visible: root.confirmRemoval
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            Label {
                Layout.fillWidth: true
                text: qsTr("Remove this empty folder?")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyLargeFontSize
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Item {
                    Layout.fillWidth: true
                }

                SZHButton {
                    text: qsTr("Cancel")
                    variant: "secondary"
                    onClicked: root.confirmRemoval = false
                }

                SZHButton {
                    objectName: "confirmCollectionRemovalButton"
                    text: qsTr("Remove")
                    onClicked: root.removeFolder()
                }
            }
        }
    }
}
