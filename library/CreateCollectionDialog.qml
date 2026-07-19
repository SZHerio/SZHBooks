import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    objectName: "createCollectionDialog"

    required property var syncService
    property string parentPath: ""

    signal collectionCreated(string collectionPath)

    function parentOptions() {
        var options = [{"value": "", "label": qsTr("Library root")}]
        const collections = root.syncService.collections
        for (var index = 0; index < collections.length; ++index) {
            options.push({
                "value": collections[index],
                "label": collections[index].split("/").join("  /  ")
            })
        }
        return options
    }

    function openFor(collectionPath) {
        root.parentPath = collectionPath === "." ? "" : collectionPath
        folderName.text = ""
        root.open()
    }

    function createFolder() {
        const cleanName = folderName.text.trim()
        if (cleanName.length === 0
                || !root.syncService.createCollection(root.parentPath, cleanName)) {
            return
        }
        const path = root.parentPath.length > 0
                   ? root.parentPath + "/" + cleanName
                   : cleanName
        root.collectionCreated(path)
        root.close()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.max(0, Math.min(420,
                                (parent ? parent.width : 452) - Theme.spaceXl))
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
            text: qsTr("New folder")
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.titleFontSize
            font.weight: Font.DemiBold
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                text: qsTr("Parent folder")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            SZHMenuButton {
                objectName: "collectionParentMenu"
                Layout.fillWidth: true
                value: root.parentPath
                options: root.parentOptions()
                onValueSelected: value => root.parentPath = value
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                text: qsTr("Folder name")
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            SZHTextField {
                id: folderName

                objectName: "collectionNameField"
                Layout.fillWidth: true
                placeholderText: qsTr("For example, Fiction")
                Keys.onReturnPressed: root.createFolder()
                Keys.onEnterPressed: root.createFolder()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Item {
                Layout.fillWidth: true
            }

            SZHButton {
                objectName: "cancelCollectionButton"
                text: qsTr("Cancel")
                variant: "secondary"
                onClicked: root.close()
            }

            SZHButton {
                objectName: "createCollectionButton"
                text: qsTr("Create")
                enabled: folderName.text.trim().length > 0
                onClicked: root.createFolder()
            }
        }
    }

    onOpened: folderName.forceActiveFocus()
}
