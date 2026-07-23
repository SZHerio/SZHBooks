import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var libraryModel
    required property var syncService

    signal collectionActionsRequested

    width: Math.min(720, parent ? parent.width - Theme.spaceXl * 2 : 720)
    padding: Theme.spaceMd
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: Theme.strongBorderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: qsTr("Library options")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyLargeFontSize
                font.weight: Font.DemiBold
            }

            SZHIconButton {
                symbol: "\u00d7"
                toolTip: qsTr("Close")
                onClicked: root.close()
            }
        }

        LibraryControls {
            Layout.fillWidth: true
            libraryModel: root.libraryModel
            syncService: root.syncService
            showCollectionControl: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            SZHButton {
                visible: root.libraryModel.collectionFilter.length > 0
                         && root.libraryModel.collectionFilter !== "."
                text: qsTr("Folder actions")
                variant: "secondary"
                onClicked: {
                    root.close()
                    root.collectionActionsRequested()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            SZHButton {
                visible: root.libraryModel.hasActiveFilters
                text: qsTr("Clear filters")
                variant: "secondary"
                onClicked: root.libraryModel.clearFilters()
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: Theme.motionFast
            }
            NumberAnimation {
                property: "scale"
                from: 0.98
                to: 1
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Theme.motionFast
        }
    }
}
