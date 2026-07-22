pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var syncService
    property string selectedCollection: ""

    signal collectionSelected(string collectionPath)
    signal createCollectionRequested(string parentPath)
    signal bookDropped(url sourceUrl, string collectionPath)

    function collectionOptions() {
        var options = [
            {
                "value": "",
                "label": qsTr("All books"),
                "depth": 0
            },
            {
                "value": ".",
                "label": qsTr("Library root"),
                "depth": 0
            }
        ]
        const collections = root.syncService.collections
        for (var index = 0; index < collections.length; ++index) {
            const parts = collections[index].split("/")
            options.push({
                "value": collections[index],
                "label": parts[parts.length - 1],
                "depth": parts.length - 1
            })
        }
        return options
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.borderColor
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.rightMargin: Theme.spaceMd
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                Layout.fillWidth: true
                text: qsTr("Folders")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                font.weight: Font.DemiBold
            }

            SZHIconButton {
                symbol: "+"
                enabled: root.syncService.available && !root.syncService.fileService.busy
                toolTip: qsTr("New folder")
                onClicked: root.createCollectionRequested(root.selectedCollection === "." ? "" : root.selectedCollection)
            }
        }

        ListView {
            id: collectionList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.space2xs
            model: root.collectionOptions()
            ScrollBar.vertical: SZHScrollBar {}

            delegate: Rectangle {
                id: collectionDelegate

                required property int index
                required property var modelData

                width: ListView.view.width
                height: Theme.compactControlHeight
                color: collectionDropArea.containsDrag || root.selectedCollection === modelData.value ? Theme.surfaceMutedColor : "transparent"
                radius: Theme.radiusSm
                border.color: collectionDropArea.containsDrag ? Theme.strongBorderColor : "transparent"
                border.width: collectionDropArea.containsDrag ? 1 : 0

                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: Theme.spaceSm + Math.min(3, modelData.depth) * Theme.spaceSm
                    anchors.rightMargin: Theme.spaceXs
                    text: modelData.label
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyFontSize
                    font.weight: root.selectedCollection === modelData.value ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                }

                TapHandler {
                    onTapped: root.collectionSelected(modelData.value)
                }

                DropArea {
                    id: collectionDropArea

                    anchors.fill: parent
                    keys: ["szhbooks-book"]
                    onDropped: drop => {
                        if (!drop.source || !drop.source.sourceUrl)
                            return
                        const destination = modelData.value === "." ? "" : modelData.value
                        root.bookDropped(drop.source.sourceUrl, destination)
                        drop.acceptProposedAction()
                    }
                }
            }
        }
    }
}
