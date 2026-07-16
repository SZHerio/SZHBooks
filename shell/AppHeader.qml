import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var readerController
    required property var readerWorkspace
    required property var settingsStore
    property bool darkMode: false
    property bool showingLibrary: false

    signal openRequested
    signal libraryRequested
    signal darkModeToggled(bool darkMode)

    function closeReaderPopups() {
        searchPopup.close()
        annotationsPopup.close()
        readingSettings.close()
        chapterNavigation.close()
    }

    function openSearch() {
        if (root.showingLibrary || !root.readerWorkspace.hasDocument) {
            return
        }
        annotationsPopup.close()
        readingSettings.close()
        chapterNavigation.close()
        searchPopup.openAndFocus()
    }

    function openAnnotations(tab) {
        if (root.showingLibrary || !root.readerWorkspace.hasDocument) {
            return
        }
        searchPopup.close()
        readingSettings.close()
        chapterNavigation.close()
        annotationsPopup.activeTab = tab === "highlights" ? "highlights" : "bookmarks"
        annotationsPopup.open()
    }

    implicitHeight: Theme.toolbarHeight
    color: Theme.chromeColor

    onShowingLibraryChanged: {
        if (root.showingLibrary) {
            root.closeReaderPopups()
        }
    }

    Connections {
        target: root.readerWorkspace

        function onHasChaptersChanged() {
            if (!root.readerWorkspace.hasChapters) {
                chapterNavigation.close()
            }
        }

        function onHasDocumentChanged() {
            if (!root.readerWorkspace.hasDocument) {
                root.closeReaderPopups()
            }
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: Theme.motionNormal
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        spacing: Theme.spaceSm

        SZHBrandLogo {
            iconOnly: root.width < 940
            darkVariant: Theme.darkMode
            Layout.preferredWidth: iconOnly ? 42 : 176
            Layout.preferredHeight: 42
            Layout.rightMargin: Theme.spaceXs
        }

        SZHIconButton {
            symbol: "\u25a6"
            toolTip: qsTr("Library")
            onChrome: true
            selected: root.showingLibrary
            onClicked: root.libraryRequested()
        }

        SZHButton {
            text: root.showingLibrary ? qsTr("Add book") : qsTr("Open book")
            variant: "chrome"
            onClicked: root.openRequested()
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceSm
            Layout.rightMargin: Theme.spaceSm
            text: root.showingLibrary
                  ? qsTr("Library")
                  : root.readerController.title
                    + (root.readerController.author.length > 0
                       ? qsTr("  \u00b7  ") + root.readerController.author
                       : "")
            color: Theme.chromeMutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyFontSize
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            Layout.preferredWidth: 1
            Layout.preferredHeight: 24
            Layout.leftMargin: Theme.spaceXs
            Layout.rightMargin: Theme.spaceXs
            color: Theme.chromeBorderColor
        }

        SZHIconButton {
            id: searchButton

            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "\u2315"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: qsTr("Search in book")
            onChrome: true
            selected: searchPopup.opened
            onClicked: searchPopup.opened ? searchPopup.close() : root.openSearch()
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: root.readerWorkspace.currentLocationBookmarked ? "\u2605" : "\u2606"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: root.readerWorkspace.currentLocationBookmarked
                     ? qsTr("Remove bookmark")
                     : qsTr("Add bookmark")
            onChrome: true
            selected: root.readerWorkspace.currentLocationBookmarked
            onClicked: root.readerWorkspace.toggleCurrentBookmark()
        }

        SZHIconButton {
            id: annotationsButton

            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "\u2261"
            toolTip: qsTr("Reading marks")
            onChrome: true
            selected: annotationsPopup.opened
            onClicked: annotationsPopup.opened
                       ? annotationsPopup.close()
                       : root.openAnnotations("bookmarks")
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "-"
            toolTip: root.readerWorkspace.showingPdf
                         ? qsTr("Zoom out")
                         : qsTr("Decrease font size")
            onChrome: true
            enabled: root.readerWorkspace.canDecreaseScale
            onClicked: root.readerWorkspace.decreaseScale()
        }

        Label {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            text: root.readerWorkspace.scaleLabel
            color: Theme.chromeMutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.preferredWidth: 58
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.hasDocument
            symbol: "+"
            toolTip: root.readerWorkspace.showingPdf
                         ? qsTr("Zoom in")
                         : qsTr("Increase font size")
            onChrome: true
            enabled: root.readerWorkspace.canIncreaseScale
            onClicked: root.readerWorkspace.increaseScale()
        }

        SZHIconButton {
            visible: !root.showingLibrary && root.readerWorkspace.showingPdf
            symbol: "\u2922"
            toolTip: qsTr("Fit page to width")
            onChrome: true
            onClicked: root.readerWorkspace.fitPdfToWidth()
        }

        SZHIconButton {
            id: chapterButton

            visible: !root.showingLibrary && root.readerWorkspace.hasChapters
            symbol: "\u2630"
            toolTip: qsTr("Chapters")
            onChrome: true
            selected: chapterNavigation.opened
            onClicked: {
                searchPopup.close()
                annotationsPopup.close()
                readingSettings.close()
                chapterNavigation.opened
                    ? chapterNavigation.close()
                    : chapterNavigation.open()
            }
        }

        SZHIconButton {
            visible: !root.showingLibrary
            symbol: "\u2699"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: qsTr("Reading settings")
            onChrome: true
            selected: readingSettings.opened
            onClicked: {
                searchPopup.close()
                annotationsPopup.close()
                chapterNavigation.close()
                readingSettings.opened
                    ? readingSettings.close()
                    : readingSettings.open()
            }
        }

        SZHIconButton {
            symbol: root.darkMode ? "\u2600" : "\u263e"
            symbolPixelSize: Theme.bodyLargeFontSize
            toolTip: root.darkMode ? qsTr("Use light theme") : qsTr("Use dark theme")
            onChrome: true
            onClicked: root.darkModeToggled(!root.darkMode)
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
        x: Math.max(Theme.spaceMd, root.width - width - Theme.spaceLg)
        y: root.height + Theme.spaceXs
        settingsStore: root.settingsStore
        textSettingsAvailable: root.readerWorkspace.showingText
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

    AnnotationsPopup {
        id: annotationsPopup

        parent: root
        x: Math.max(Theme.spaceMd, root.width - width - Theme.spaceLg)
        y: root.height + Theme.spaceXs
        readerWorkspace: root.readerWorkspace
        annotationStore: root.readerWorkspace.annotationStore
        onAboutToShow: {
            const buttonPosition = annotationsButton.mapToItem(root, 0, 0)
            x = Math.max(Theme.spaceMd,
                         Math.min(root.width - width - Theme.spaceMd,
                                  buttonPosition.x + annotationsButton.width - width))
        }
    }

    ChapterNavigationPopup {
        id: chapterNavigation

        parent: root
        x: Theme.spaceMd
        y: root.height + Theme.spaceXs
        readerWorkspace: root.readerWorkspace
        onAboutToShow: {
            const buttonPosition = chapterButton.mapToItem(root, 0, 0)
            x = Math.max(Theme.spaceMd,
                         buttonPosition.x + chapterButton.width - width)
        }
    }
}
