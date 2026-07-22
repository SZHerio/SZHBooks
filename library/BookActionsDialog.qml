import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var fileService
    required property var syncService
    property url sourceUrl
    property string bookTitle: ""
    property string collectionPath: ""
    property bool fileAvailable: false
    property string confirmMode: ""
    readonly property bool managedBook: root.fileService.isManagedBook(root.sourceUrl)

    signal bookUpdated(url sourceUrl)
    signal bookRemoved

    function collectionOptions() {
        var options = [
            {
                "value": "",
                "label": qsTr("Library root")
            }
        ]
        const collections = root.syncService.collections
        for (var index = 0; index < collections.length; ++index) {
            options.push({
                "value": collections[index],
                "label": collections[index].split("/").join("  /  ")
            })
        }
        return options
    }

    function openFor(sourceUrl, title, collectionPath, fileAvailable) {
        root.sourceUrl = sourceUrl
        root.bookTitle = title
        root.collectionPath = collectionPath
        root.fileAvailable = fileAvailable
        root.confirmMode = ""
        destinationMenu.value = collectionPath
        renameField.text = root.fileService.fileBaseName(sourceUrl)
        root.open()
    }

    function moveBook() {
        const movedUrl = root.fileService.moveBook(root.sourceUrl, destinationMenu.value)
        if (movedUrl.toString().length === 0)
            return
        root.sourceUrl = movedUrl
        root.collectionPath = destinationMenu.value
        root.bookUpdated(movedUrl)
        root.close()
    }

    function renameBook() {
        const renamedUrl = root.fileService.renameBook(root.sourceUrl, renameField.text)
        if (renamedUrl.toString().length === 0)
            return
        root.sourceUrl = renamedUrl
        root.bookUpdated(renamedUrl)
        root.close()
    }

    function confirmRemoval() {
        const deleteFile = root.confirmMode === "delete"
        if (!root.fileService.removeBook(root.sourceUrl, deleteFile))
            return
        root.bookRemoved()
        root.close()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.max(Theme.spaceMd, Math.round((parent.height - height) / 2)) : 0
    width: Math.max(0, Math.min(500, (parent ? parent.width : 532) - Theme.spaceXl))
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
                    Layout.fillWidth: true
                    text: root.bookTitle
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.titleFontSize
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: root.collectionPath.length > 0 ? root.collectionPath : qsTr("Library root")
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
            visible: root.confirmMode.length === 0
            Layout.fillWidth: true
            spacing: Theme.spaceLg

            ColumnLayout {
                visible: root.managedBook && root.fileAvailable
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Label {
                    text: qsTr("Move to folder")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    SZHMenuButton {
                        id: destinationMenu

                        objectName: "bookDestinationMenu"
                        Layout.fillWidth: true
                        value: root.collectionPath
                        options: root.collectionOptions()
                    }

                    SZHButton {
                        objectName: "moveBookButton"
                        text: qsTr("Move")
                        variant: "secondary"
                        enabled: destinationMenu.value !== root.collectionPath
                        onClicked: root.moveBook()
                    }
                }
            }

            ColumnLayout {
                visible: root.managedBook && root.fileAvailable
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                Label {
                    text: qsTr("File name")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    SZHTextField {
                        id: renameField

                        objectName: "bookRenameField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Book name")
                        Keys.onReturnPressed: root.renameBook()
                        Keys.onEnterPressed: root.renameBook()
                    }

                    SZHButton {
                        objectName: "renameBookButton"
                        text: qsTr("Rename")
                        variant: "secondary"
                        enabled: renameField.text.trim().length > 0
                        onClicked: root.renameBook()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.borderColor
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceXs

                SZHButton {
                    objectName: "removeBookRecordButton"
                    text: qsTr("Remove from library")
                    variant: "secondary"
                    onClicked: root.confirmMode = "remove"
                }

                SZHButton {
                    objectName: "deleteBookFileButton"
                    visible: root.managedBook && root.fileAvailable
                    text: qsTr("Move file to Trash")
                    variant: "secondary"
                    onClicked: root.confirmMode = "delete"
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        ColumnLayout {
            visible: root.confirmMode.length > 0
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            Label {
                Layout.fillWidth: true
                text: root.confirmMode === "delete" ? qsTr("Move this book file to the Trash?") : qsTr("Remove this book from the library?")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyLargeFontSize
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                text: root.confirmMode === "delete" ? qsTr("Reading progress and notes stay in your profile.") : qsTr("The book file will stay in its current folder.")
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
                    text: qsTr("Cancel")
                    variant: "secondary"
                    onClicked: root.confirmMode = ""
                }

                SZHButton {
                    objectName: "confirmBookRemovalButton"
                    text: root.confirmMode === "delete" ? qsTr("Move to Trash") : qsTr("Remove")
                    onClicked: root.confirmRemoval()
                }
            }
        }
    }
}
