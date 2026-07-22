pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var libraryModel
    required property var syncService
    readonly property var fileService: root.syncService.fileService

    signal addRequested
    signal openRequested(url sourceUrl)
    signal relinkRequested(url sourceUrl)
    signal chooseFolderRequested
    signal createCollectionRequested(string parentPath)
    signal syncDetailsRequested
    signal filesDropped(var fileUrls)

    function fallbackColor(row) {
        const colors = ["#111111", "#2b2b2b", "#444444", "#5d5d5d", "#242424", "#505050"]
        return colors[row % colors.length]
    }

    BookActionsDialog {
        id: bookActionsDialog

        fileService: root.fileService
        syncService: root.syncService
        onBookUpdated: root.libraryModel.refresh()
        onBookRemoved: root.libraryModel.refresh()
    }

    CollectionActionsDialog {
        id: collectionActionsDialog

        fileService: root.fileService
        onCollectionRenamed: collectionPath => {
            root.libraryModel.collectionFilter = collectionPath
            root.libraryModel.refresh()
        }
        onCollectionRemoved: {
            root.libraryModel.collectionFilter = ""
            root.libraryModel.refresh()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.windowColor

        Behavior on color {
            ColorAnimation {
                duration: Theme.motionNormal
            }
        }
    }

    DropArea {
        id: externalBookDropArea

        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: drop => {
            if (!drop.hasUrls)
                return
            root.filesDropped(drop.urls)
            drop.acceptProposedAction()
        }
    }

    Rectangle {
        z: 50
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        visible: externalBookDropArea.containsDrag
        color: "transparent"
        border.color: Theme.strongBorderColor
        border.width: 2
        radius: Theme.radiusLg
    }

    ColumnLayout {
        id: libraryToolbar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceLg
        anchors.leftMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.rightMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2xs

                Label {
                    text: qsTr("Library")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.displayFontSize
                    font.weight: Font.DemiBold
                }

                Label {
                    text: qsTr("%n book(s)", "", root.libraryModel.totalCount)
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                }
            }

            SZHTextField {
                Layout.preferredWidth: root.width < 900 ? 180 : 300
                Layout.minimumWidth: 140
                placeholderText: qsTr("Search library")
                text: root.libraryModel.filterText
                onTextEdited: root.libraryModel.filterText = text
            }

            SZHIconButton {
                enabled: root.syncService.configured && root.syncService.available && !root.fileService.busy
                symbol: "+"
                toolTip: qsTr("New folder")
                onClicked: root.createCollectionRequested(root.libraryModel.collectionFilter)
            }

            SZHIconButton {
                enabled: root.libraryModel.collectionFilter.length > 0 && root.libraryModel.collectionFilter !== "." && !root.fileService.busy
                symbol: "\u22ef"
                toolTip: qsTr("Folder actions")
                onClicked: collectionActionsDialog.openFor(root.libraryModel.collectionFilter)
            }

            SZHButton {
                text: qsTr("Add book")
                symbol: "+"
                onClicked: root.addRequested()
            }
        }

        LibraryControls {
            Layout.fillWidth: true
            libraryModel: root.libraryModel
            syncService: root.syncService
            showCollectionControl: root.width < 960
        }

        LibrarySyncBar {
            Layout.fillWidth: true
            syncService: root.syncService
            onChooseFolderRequested: root.chooseFolderRequested()
            onDetailsRequested: root.syncDetailsRequested()
        }

        LibraryImportProgress {
            Layout.fillWidth: true
            fileService: root.fileService
        }
    }

    Item {
        id: contentArea

        anchors.top: libraryToolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spaceLg
        anchors.leftMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.rightMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.bottomMargin: Theme.spaceLg

        LibraryCollectionSidebar {
            id: collectionSidebar

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 210
            visible: root.width >= 960 && root.syncService.configured
            syncService: root.syncService
            selectedCollection: root.libraryModel.collectionFilter
            onCollectionSelected: collectionPath => {
                root.libraryModel.collectionFilter = collectionPath
            }
            onCreateCollectionRequested: parentPath => {
                root.createCollectionRequested(parentPath)
            }
            onBookDropped: (sourceUrl, collectionPath) => {
                const movedUrl = root.fileService.moveBook(sourceUrl, collectionPath)
                if (movedUrl.toString().length > 0) {
                    root.libraryModel.collectionFilter = collectionPath
                    root.libraryModel.refresh()
                }
            }
        }

        Item {
            id: booksContent

            anchors.top: parent.top
            anchors.left: collectionSidebar.visible ? collectionSidebar.right : parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: collectionSidebar.visible ? Theme.spaceLg : 0

            ColumnLayout {
                anchors.fill: parent
                visible: root.libraryModel.totalCount > 0
                spacing: Theme.spaceSm

                Label {
                    visible: !root.libraryModel.hasActiveFilters
                    text: qsTr("Continue reading")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyLargeFontSize
                    font.weight: Font.DemiBold
                }

                LibraryContinueCard {
                    visible: !root.libraryModel.hasActiveFilters
                    Layout.fillWidth: true
                    book: root.libraryModel.recentBook
                    onOpenRequested: sourceUrl => root.openRequested(sourceUrl)
                    onRelinkRequested: sourceUrl => root.relinkRequested(sourceUrl)
                }

                Label {
                    text: root.libraryModel.hasActiveFilters ? qsTr("Results") : qsTr("All books")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyLargeFontSize
                    font.weight: Font.DemiBold
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    GridView {
                        id: bookGrid

                        readonly property real availableGridWidth: Math.max(1, width)
                        readonly property int columnCount: Math.max(1, Math.floor(availableGridWidth / 188))

                        anchors.fill: parent
                        clip: true
                        model: root.libraryModel
                        cellWidth: Math.floor(availableGridWidth / columnCount)
                        cellHeight: 300
                        boundsBehavior: Flickable.StopAtBounds
                        visible: root.libraryModel.count > 0 && root.libraryModel.viewMode === "grid"
                        ScrollBar.vertical: SZHScrollBar {}

                        delegate: LibraryBookDelegate {
                            width: bookGrid.cellWidth - Theme.spaceSm
                            height: bookGrid.cellHeight - Theme.spaceSm
                            fallbackColor: root.fallbackColor(index)
                            onOpenRequested: sourceUrl => root.openRequested(sourceUrl)
                            onRelinkRequested: sourceUrl => root.relinkRequested(sourceUrl)
                            onActionsRequested: (sourceUrl, title, collectionPath, fileAvailable) => {
                                bookActionsDialog.openFor(sourceUrl, title, collectionPath, fileAvailable)
                            }
                        }
                    }

                    ListView {
                        id: bookList

                        anchors.fill: parent
                        clip: true
                        model: root.libraryModel
                        spacing: Theme.spaceXs
                        boundsBehavior: Flickable.StopAtBounds
                        visible: root.libraryModel.count > 0 && root.libraryModel.viewMode === "list"
                        ScrollBar.vertical: SZHScrollBar {}

                        delegate: LibraryBookListDelegate {
                            width: bookList.width
                            height: 82
                            fallbackColor: root.fallbackColor(index)
                            onOpenRequested: sourceUrl => root.openRequested(sourceUrl)
                            onRelinkRequested: sourceUrl => root.relinkRequested(sourceUrl)
                            onActionsRequested: (sourceUrl, title, collectionPath, fileAvailable) => {
                                bookActionsDialog.openFor(sourceUrl, title, collectionPath, fileAvailable)
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(420, booksContent.width - Theme.space2xl * 2)
                visible: root.libraryModel.totalCount === 0
                spacing: Theme.spaceMd

                SZHBrandLogo {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 76
                    iconOnly: true
                    darkVariant: Theme.darkMode
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Your library is empty")
                    color: Theme.textColor
                    font.family: Theme.readingFontFamily
                    font.pixelSize: Theme.displayFontSize
                    font.weight: Font.DemiBold
                }

                SZHButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Add book")
                    symbol: "+"
                    onClicked: root.addRequested()
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                visible: root.libraryModel.totalCount > 0 && root.libraryModel.count === 0
                spacing: Theme.spaceMd

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("No books match these filters")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyLargeFontSize
                }

                SZHButton {
                    Layout.alignment: Qt.AlignHCenter
                    variant: "secondary"
                    text: qsTr("Clear filters")
                    onClicked: root.libraryModel.clearFilters()
                }
            }
        }
    }
}
