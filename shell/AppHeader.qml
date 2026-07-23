import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var readerController
    required property var readerWorkspace
    required property var settingsStore
    required property var localizationController
    required property var diagnosticService
    required property var libraryModel
    required property var syncService
    property bool darkMode: false
    property bool showingLibrary: false
    readonly property bool compactMode: width < 1080
    readonly property bool compactBrand: width < 760
    readonly property bool readerPopupOpen: searchPopup.opened
                                                || readingSettings.opened
                                                || readerTools.opened
                                                || applicationMenu.opened

    signal openRequested
    signal libraryRequested
    signal darkModeToggled(bool darkMode)
    signal focusModeRequested
    signal backupProfileRequested
    signal restoreProfileRequested
    signal notesCenterRequested
    signal librarySearchRequested
    signal chooseFolderRequested
    signal createCollectionRequested
    signal selectionModeRequested
    signal syncDetailsRequested
    signal aboutRequested

    function closeReaderPopups() {
        searchPopup.close()
        readingSettings.close()
        readerTools.close()
        applicationMenu.close()
    }

    function openSearch() {
        if (root.showingLibrary || !root.readerWorkspace.hasDocument)
            return
        root.closeReaderPopups()
        searchPopup.openAndFocus()
    }

    function openAnnotations(tab) {
        if (root.showingLibrary || !root.readerWorkspace.hasDocument)
            return
        root.closeReaderPopups()
        root.readerWorkspace.openSidebar(tab === "highlights" ? "notes" : "bookmarks")
    }

    function toggleChapterNavigation() {
        root.closeReaderPopups()
        root.readerWorkspace.toggleSidebar("contents")
    }

    function toggleReadingSettings() {
        const wasOpen = readingSettings.opened
        root.closeReaderPopups()
        if (!wasOpen)
            readingSettings.open()
    }

    function toggleReaderTools() {
        const wasOpen = readerTools.opened
        root.closeReaderPopups()
        if (!wasOpen)
            readerTools.open()
    }

    function toggleApplicationMenu() {
        const wasOpen = applicationMenu.opened
        root.closeReaderPopups()
        if (!wasOpen)
            applicationMenu.open()
    }

    implicitHeight: Theme.toolbarHeight
    color: Theme.chromeColor

    onShowingLibraryChanged: {
        root.closeReaderPopups()
        if (root.showingLibrary)
            root.readerWorkspace.closeSidebar()
    }

    Connections {
        target: root.readerWorkspace

        function onHasChaptersChanged() {
            if (!root.readerWorkspace.hasChapters
                    && root.readerWorkspace.sidebarTab === "contents") {
                root.readerWorkspace.closeSidebar()
            }
        }

        function onHasDocumentChanged() {
            if (!root.readerWorkspace.hasDocument)
                root.closeReaderPopups()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceLg
        spacing: Theme.spaceSm

        Button {
            id: brandMenuButton

            objectName: "brandMenuButton"
            readonly property bool interactionActive: hovered || down || applicationMenu.opened
            Layout.preferredWidth: root.compactBrand ? 50 : 190
            Layout.preferredHeight: 46
            padding: 0
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus
            Accessible.role: Accessible.ButtonMenu
            Accessible.name: qsTr("SZHBooks menu")
            onClicked: root.toggleApplicationMenu()

            ToolTip.text: qsTr("SZHBooks menu")
            ToolTip.visible: hovered
            ToolTip.delay: Theme.tooltipDelay

            contentItem: Item {
                SZHBrandLogo {
                    anchors.left: parent.left
                    anchors.leftMargin: root.compactBrand ? Theme.space2xs : Theme.spaceXs
                    anchors.right: menuChevron.left
                    anchors.rightMargin: Theme.space2xs
                    anchors.verticalCenter: parent.verticalCenter
                    height: 38
                    iconOnly: root.compactBrand
                    darkVariant: brandMenuButton.interactionActive
                                 ? !Theme.darkMode : Theme.darkMode
                }

                Text {
                    id: menuChevron

                    anchors.right: parent.right
                    anchors.rightMargin: Theme.spaceXs
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.iconFontSize
                    text: applicationMenu.opened ? "\u25b4" : "\u25be"
                    color: brandMenuButton.interactionActive
                           ? Theme.interactionTextColor : Theme.chromeMutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            background: Rectangle {
                color: brandMenuButton.interactionActive
                       ? Theme.interactionColor : "transparent"
                radius: Theme.radiusMd
                border.color: brandMenuButton.activeFocus
                              ? Theme.focusColor : "transparent"
                border.width: brandMenuButton.activeFocus ? 2 : 0
            }
        }

        Label {
            visible: !root.showingLibrary
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceSm
            Layout.rightMargin: Theme.spaceSm
            text: root.readerController.title
                  + (root.readerController.author.length > 0
                     ? qsTr("  \u00b7  ") + root.readerController.author : "")
            color: Theme.chromeMutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyFontSize
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Item {
            visible: root.showingLibrary
            Layout.fillWidth: true
        }

        SZHIconButton {
            id: searchButton

            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "\u2315"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: qsTr("Search in book (Ctrl+F)")
            onChrome: true
            selected: searchPopup.opened
            onClicked: searchPopup.opened ? searchPopup.close() : root.openSearch()
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: root.readerWorkspace.currentLocationBookmarked ? "\u2605" : "\u2606"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: root.readerWorkspace.currentLocationBookmarked
                     ? qsTr("Remove bookmark (Ctrl+B)")
                     : qsTr("Add bookmark (Ctrl+B)")
            onChrome: true
            selected: root.readerWorkspace.currentLocationBookmarked
            onClicked: root.readerWorkspace.toggleCurrentBookmark()
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "\u2261"
            toolTip: qsTr("Reading marks")
            onChrome: true
            selected: root.readerWorkspace.sidebarOpen
                      && (root.readerWorkspace.sidebarTab === "bookmarks"
                          || root.readerWorkspace.sidebarTab === "notes")
            onClicked: root.readerWorkspace.toggleSidebar("bookmarks")
        }

        SZHIconButton {
            visible: !root.compactMode && !root.showingLibrary
                     && root.readerWorkspace.hasDocument
            symbol: "-"
            toolTip: root.readerWorkspace.showingPdf
                         ? qsTr("Zoom out") : qsTr("Decrease font size")
            onChrome: true
            enabled: root.readerWorkspace.canDecreaseScale
            onClicked: root.readerWorkspace.decreaseScale()
        }

        Label {
            visible: !root.compactMode && !root.showingLibrary
                     && root.readerWorkspace.hasDocument
            text: root.readerWorkspace.scaleLabel
            color: Theme.chromeMutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.preferredWidth: 58
        }

        SZHIconButton {
            visible: !root.compactMode && !root.showingLibrary
                     && root.readerWorkspace.hasDocument
            symbol: "+"
            toolTip: root.readerWorkspace.showingPdf
                         ? qsTr("Zoom in") : qsTr("Increase font size")
            onChrome: true
            enabled: root.readerWorkspace.canIncreaseScale
            onClicked: root.readerWorkspace.increaseScale()
        }

        SZHIconButton {
            visible: !root.compactMode && !root.showingLibrary
                     && root.readerWorkspace.showingPdf
            symbol: "\u2922"
            toolTip: qsTr("Fit page to width")
            onChrome: true
            onClicked: root.readerWorkspace.fitPdfToWidth()
        }

        SZHIconButton {
            id: chapterButton

            visible: !root.compactMode && !root.showingLibrary
                     && root.readerWorkspace.hasChapters
            symbol: "\u2630"
            toolTip: qsTr("Chapters")
            onChrome: true
            selected: root.readerWorkspace.sidebarOpen
                      && root.readerWorkspace.sidebarTab === "contents"
            onClicked: root.toggleChapterNavigation()
        }

        SZHIconButton {
            id: overflowButton

            visible: root.compactMode && !root.showingLibrary
            symbol: "\u22ef"
            symbolPixelSize: Theme.titleFontSize
            toolTip: qsTr("Reader tools")
            onChrome: true
            selected: readerTools.opened
            onClicked: root.toggleReaderTools()
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "\u26f6"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: qsTr("Focus mode (F11)")
            onChrome: true
            onClicked: root.focusModeRequested()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.chromeBorderColor
    }

    ReadingSettingsPopup {
        id: readingSettings

        parent: root
        x: Theme.spaceMd
        y: root.height + Theme.spaceXs
        settingsStore: root.settingsStore
        localizationController: root.localizationController
        diagnosticService: root.diagnosticService
        readerWorkspace: root.readerWorkspace
        textSettingsAvailable: !root.showingLibrary && root.readerWorkspace.showingText
        onBackupRequested: root.backupProfileRequested()
        onRestoreRequested: root.restoreProfileRequested()
    }

    SearchPopup {
        id: searchPopup

        parent: root
        x: Math.max(Theme.spaceMd, root.width - width - Theme.spaceLg)
        y: root.height + Theme.spaceXs
        readerWorkspace: root.readerWorkspace
        onAboutToShow: {
            const buttonPosition = searchButton.mapToItem(root, 0, 0)
            x = Math.max(Theme.spaceMd,
                         Math.min(root.width - width - Theme.spaceMd,
                                  buttonPosition.x + searchButton.width - width))
        }
    }

    ReaderToolsPopup {
        id: readerTools

        parent: root
        x: Math.max(Theme.spaceMd, root.width - width - Theme.spaceLg)
        y: root.height + Theme.spaceXs
        readerWorkspace: root.readerWorkspace
        onAboutToShow: {
            const buttonPosition = overflowButton.mapToItem(root, 0, 0)
            x = Math.max(Theme.spaceMd,
                         Math.min(root.width - width - Theme.spaceMd,
                                  buttonPosition.x + overflowButton.width - width))
        }
        onChaptersRequested: root.toggleChapterNavigation()
    }

    ApplicationMenuPopup {
        id: applicationMenu

        parent: root
        x: Theme.spaceMd
        y: root.height + Theme.spaceXs
        syncService: root.syncService
        showingLibrary: root.showingLibrary
        libraryHasBooks: root.libraryModel.totalCount > 0
        selectionMode: root.libraryModel.selectionMode
        onOpenRequested: root.openRequested()
        onLibraryRequested: root.libraryRequested()
        onLibrarySearchRequested: root.librarySearchRequested()
        onNotesCenterRequested: root.notesCenterRequested()
        onSelectionModeRequested: root.selectionModeRequested()
        onCreateCollectionRequested: root.createCollectionRequested()
        onChooseFolderRequested: root.chooseFolderRequested()
        onSyncDetailsRequested: root.syncDetailsRequested()
        onSettingsRequested: root.toggleReadingSettings()
        onAboutRequested: root.aboutRequested()
    }
}
