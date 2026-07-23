pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var libraryModel
    required property var syncService
    required property var desktopIntegration
    readonly property var fileService: root.syncService.fileService
    property var supportedDropUrls: []
    property int unsupportedDropCount: 0

    signal addRequested
    signal openRequested(url sourceUrl)
    signal relinkRequested(url sourceUrl)
    signal chooseFolderRequested
    signal createCollectionRequested(string parentPath)
    signal syncDetailsRequested
    signal filesDropped(var fileUrls)

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Library")

    function updateDropPreview(fileUrls) {
        root.supportedDropUrls = root.desktopIntegration.supportedBookUrls(fileUrls)
        root.unsupportedDropCount = 0
        for (let index = 0; index < fileUrls.length; ++index) {
            if (!root.desktopIntegration.isSupportedBookUrl(fileUrls[index]))
                root.unsupportedDropCount += 1
        }
    }

    function clearDropPreview() {
        root.supportedDropUrls = []
        root.unsupportedDropCount = 0
    }

    function activeBooksView() {
        return root.libraryModel.viewMode === "list" ? bookList : bookGrid
    }

    function focusSearch() {
        librarySearchField.forceActiveFocus(Qt.ShortcutFocusReason)
        librarySearchField.selectAll()
    }

    function focusBooks() {
        const view = root.activeBooksView()
        if (view.count <= 0)
            return
        if (view.currentIndex < 0)
            view.currentIndex = 0
        view.forceActiveFocus(Qt.ShortcutFocusReason)
    }

    function selectAllBooks() {
        if (root.libraryModel.count <= 0)
            return
        root.libraryModel.selectAllVisible()
        root.focusBooks()
    }

    function activateBookAt(row) {
        const book = root.libraryModel.bookAt(row)
        if (!book || book.sourceUrl === undefined)
            return
        if (book.fileAvailable)
            root.openRequested(book.sourceUrl)
        else
            root.relinkRequested(book.sourceUrl)
    }

    function toggleBookAt(row) {
        const book = root.libraryModel.bookAt(row)
        if (book && book.sourceUrl !== undefined)
            root.libraryModel.toggleSelection(book.sourceUrl)
    }

    function showBookActionsAt(row) {
        const book = root.libraryModel.bookAt(row)
        if (!book || book.sourceUrl === undefined)
            return
        bookActionsDialog.openFor(book.sourceUrl,
                                  book.title,
                                  book.collectionPath,
                                  book.fileAvailable)
    }

    function cancelLibraryAction() {
        if (root.libraryModel.selectionMode) {
            root.libraryModel.selectionMode = false
        } else if (librarySearchField.text.length > 0) {
            librarySearchField.clear()
        } else if (libraryOptions.opened) {
            libraryOptions.close()
        }
    }

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
        onEditMetadataRequested: sourceUrl => metadataDialog.openFor(sourceUrl)
    }

    BookMetadataDialog {
        id: metadataDialog

        libraryModel: root.libraryModel
        onMetadataUpdated: root.libraryModel.refresh()
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
            ColorAnimation { duration: Theme.motionNormal }
        }
    }

    DropArea {
        id: externalBookDropArea

        anchors.fill: parent
        keys: ["text/uri-list"]
        onEntered: drag => {
            if (drag.hasUrls)
                root.updateDropPreview(drag.urls)
            drag.accepted = drag.hasUrls
        }
        onExited: root.clearDropPreview()
        onDropped: drop => {
            if (!drop.hasUrls) {
                root.clearDropPreview()
                return
            }
            root.updateDropPreview(drop.urls)
            const acceptedUrls = root.supportedDropUrls
            root.clearDropPreview()
            if (acceptedUrls.length === 0)
                return
            root.filesDropped(acceptedUrls)
            drop.accept(Qt.CopyAction)
        }
    }

    ColumnLayout {
        id: libraryTopArea

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceSm
        anchors.leftMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.rightMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        spacing: Theme.spaceXs

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.libraryToolbarHeight
            spacing: Theme.spaceSm

            Label {
                text: qsTr("Library")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.titleFontSize
                font.weight: Font.DemiBold
            }

            Label {
                text: root.libraryModel.totalCount
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
            }

            Item {
                Layout.fillWidth: true
            }

            SZHTextField {
                id: librarySearchField

                objectName: "libraryFilterField"
                Layout.preferredWidth: root.width < 900 ? 220 : 300
                Layout.minimumWidth: 160
                placeholderText: qsTr("Search library")
                accessibleName: qsTr("Filter library books")
                text: root.libraryModel.filterText
                onTextEdited: root.libraryModel.filterText = text
                Keys.onDownPressed: root.focusBooks()
            }

            SZHIconButton {
                id: libraryOptionsButton

                objectName: "libraryOptionsButton"
                symbol: "\u2637"
                toolTip: qsTr("Library options")
                selected: libraryOptions.opened || root.libraryModel.hasActiveFilters
                onClicked: libraryOptions.opened
                           ? libraryOptions.close() : libraryOptions.open()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: selectionRow.implicitHeight + Theme.spaceSm * 2
            visible: root.libraryModel.selectionMode
            color: Theme.surfaceColor
            border.color: Theme.interactionColor
            border.width: 1
            radius: Theme.radiusMd

            RowLayout {
                id: selectionRow

                anchors.fill: parent
                anchors.margins: Theme.spaceSm
                spacing: Theme.spaceXs

                Label {
                    Layout.fillWidth: true
                    text: qsTr("%n book(s) selected", "", root.libraryModel.selectionCount)
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.bodyFontSize
                    font.weight: Font.DemiBold
                }

                SZHButton {
                    text: qsTr("Select all")
                    variant: "secondary"
                    onClicked: root.libraryModel.selectAllVisible()
                }

                SZHButton {
                    text: qsTr("Edit details")
                    enabled: root.libraryModel.selectionCount > 0
                    onClicked: metadataDialog.openForSelection()
                }

                SZHIconButton {
                    symbol: "\u00d7"
                    toolTip: qsTr("Finish selecting")
                    onClicked: root.libraryModel.selectionMode = false
                }
            }
        }

        LibraryImportProgress {
            Layout.fillWidth: true
            fileService: root.fileService
        }

        LibraryScanProgress {
            Layout.fillWidth: true
            libraryModel: root.libraryModel
        }
    }

    Item {
        id: booksContent

        anchors.top: libraryTopArea.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: Theme.spaceSm
        anchors.leftMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.rightMargin: root.width < 900 ? Theme.spaceLg : Theme.spaceXl
        anchors.bottomMargin: Theme.spaceLg

        GridView {
            id: bookGrid

            readonly property real availableGridWidth: Math.max(1, width)
            readonly property int minimumCellWidth: root.width < 900 ? 210 : 236
            readonly property int columnCount: Math.max(1,
                                                        Math.floor(availableGridWidth
                                                                   / minimumCellWidth))

            anchors.fill: parent
            clip: true
            model: root.libraryModel
            cellWidth: Math.floor(availableGridWidth / columnCount)
            cellHeight: 370
            boundsBehavior: Flickable.StopAtBounds
            boundsMovement: Flickable.StopAtBounds
            activeFocusOnTab: true
            keyNavigationEnabled: true
            visible: root.libraryModel.count > 0 && root.libraryModel.viewMode === "grid"
            currentIndex: count > 0 ? 0 : -1
            Accessible.role: Accessible.List
            Accessible.name: qsTr("Library books, grid view")
            ScrollBar.vertical: SZHScrollBar {}

            Keys.onReturnPressed: root.activateBookAt(currentIndex)
            Keys.onEnterPressed: root.activateBookAt(currentIndex)
            Keys.onSpacePressed: root.toggleBookAt(currentIndex)
            Keys.onPressed: event => {
                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                    root.selectAllBooks()
                    event.accepted = true
                } else if (currentIndex >= 0
                           && (event.key === Qt.Key_Menu
                               || (event.key === Qt.Key_F10
                                   && (event.modifiers & Qt.ShiftModifier)))) {
                    root.showBookActionsAt(currentIndex)
                    event.accepted = true
                }
            }

            populate: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionNormal }
                    NumberAnimation { property: "scale"; from: 0.97; to: 1; duration: Theme.motionNormal; easing.type: Easing.OutCubic }
                }
            }

            delegate: LibraryBookDelegate {
                width: bookGrid.cellWidth - Theme.spaceMd
                height: bookGrid.cellHeight - Theme.spaceMd
                fallbackColor: root.fallbackColor(index)
                selectionMode: root.libraryModel.selectionMode
                keyboardCurrent: GridView.isCurrentItem && bookGrid.activeFocus
                onFocusRequested: row => {
                    bookGrid.currentIndex = row
                    bookGrid.forceActiveFocus(Qt.MouseFocusReason)
                }
                onOpenRequested: sourceUrl => root.openRequested(sourceUrl)
                onRelinkRequested: sourceUrl => root.relinkRequested(sourceUrl)
                onSelectionToggled: sourceUrl => root.libraryModel.toggleSelection(sourceUrl)
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
            boundsMovement: Flickable.StopAtBounds
            activeFocusOnTab: true
            keyNavigationEnabled: true
            visible: root.libraryModel.count > 0 && root.libraryModel.viewMode === "list"
            currentIndex: count > 0 ? 0 : -1
            Accessible.role: Accessible.List
            Accessible.name: qsTr("Library books, list view")
            ScrollBar.vertical: SZHScrollBar {}

            Keys.onReturnPressed: root.activateBookAt(currentIndex)
            Keys.onEnterPressed: root.activateBookAt(currentIndex)
            Keys.onSpacePressed: root.toggleBookAt(currentIndex)
            Keys.onPressed: event => {
                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                    root.selectAllBooks()
                    event.accepted = true
                } else if (currentIndex >= 0
                           && (event.key === Qt.Key_Menu
                               || (event.key === Qt.Key_F10
                                   && (event.modifiers & Qt.ShiftModifier)))) {
                    root.showBookActionsAt(currentIndex)
                    event.accepted = true
                }
            }

            delegate: LibraryBookListDelegate {
                width: bookList.width
                height: 82
                fallbackColor: root.fallbackColor(index)
                selectionMode: root.libraryModel.selectionMode
                keyboardCurrent: ListView.isCurrentItem && bookList.activeFocus
                onFocusRequested: row => {
                    bookList.currentIndex = row
                    bookList.forceActiveFocus(Qt.MouseFocusReason)
                }
                onOpenRequested: sourceUrl => root.openRequested(sourceUrl)
                onRelinkRequested: sourceUrl => root.relinkRequested(sourceUrl)
                onSelectionToggled: sourceUrl => root.libraryModel.toggleSelection(sourceUrl)
                onActionsRequested: (sourceUrl, title, collectionPath, fileAvailable) => {
                    bookActionsDialog.openFor(sourceUrl, title, collectionPath, fileAvailable)
                }
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            visible: root.libraryModel.totalCount === 0
            spacing: Theme.spaceSm

            SZHBrandLogo {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 84
                Layout.preferredHeight: 84
                iconOnly: true
                darkVariant: Theme.darkMode
                opacity: 0.72
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Your library is empty")
                color: Theme.mutedTextColor
                font.family: Theme.readingFontFamily
                font.pixelSize: Theme.titleFontSize
                font.weight: Font.DemiBold
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

    LibraryOptionsPopup {
        id: libraryOptions

        parent: root
        x: Math.max(Theme.spaceLg, root.width - width - Theme.spaceXl)
        y: libraryTopArea.y + Theme.libraryToolbarHeight + Theme.spaceXs
        libraryModel: root.libraryModel
        syncService: root.syncService
        onCollectionActionsRequested: {
            collectionActionsDialog.openFor(root.libraryModel.collectionFilter)
        }
    }

    Rectangle {
        z: 50
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        visible: externalBookDropArea.containsDrag
        color: Theme.surfaceColor
        opacity: 0.96
        border.color: Theme.interactionColor
        border.width: 2
        radius: Theme.radiusLg

        Accessible.role: Accessible.AlertMessage
        Accessible.name: dropStatusLabel.text

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(420, parent.width - Theme.space2xl)
            spacing: Theme.spaceSm

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.supportedDropUrls.length > 0 ? "\u2193" : "\u00d7"
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.displayFontSize
                font.weight: Font.DemiBold
            }

            Label {
                id: dropStatusLabel

                Layout.fillWidth: true
                text: root.supportedDropUrls.length > 0
                      ? qsTr("%n supported book(s) ready to add", "",
                             root.supportedDropUrls.length)
                      : qsTr("No supported book files")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.titleFontSize
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.unsupportedDropCount > 0
                text: qsTr("%n unsupported file(s) will be skipped", "",
                           root.unsupportedDropCount)
                color: Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
