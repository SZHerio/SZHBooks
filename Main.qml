import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Window

ApplicationWindow {
    id: root

    required property var readerController
    required property var localStateStore
    required property var profileArchiveService
    required property var localizationController
    required property var libraryModel
    required property var readingDocumentFormatter
    required property var readingSearchController
    required property var readingAnnotationStore
    required property var oneDriveLibraryService

    property bool showingLibrary: true
    property bool focusMode: false
    property int visibilityBeforeFocus: Window.Windowed
    property url relinkSourceUrl
    property url pendingProfileRestoreUrl
    property bool profileNoticeShown: false
    property string profileNoticeHeading
    property string profileNoticeMessage
    property string profileNoticeKind: "info"
    readonly property bool readingNavigationEnabled: !root.showingLibrary
                                                      && readerWorkspace.hasDocument
                                                      && !appHeader.readerPopupOpen
                                                      && !readerWorkspace.sidebarTextInputActive
    readonly property var supportedBookNameFilters: [
        qsTr("Supported books (*.txt *.fb2 *.epub *.pdf *.html *.htm *.md *.markdown *.docx)"),
        qsTr("Text files (*.txt *.md *.markdown *.html *.htm)"),
        qsTr("Electronic books (*.fb2 *.epub)"),
        qsTr("PDF files (*.pdf)"),
        qsTr("Word documents (*.docx)")
    ]

    width: 1120
    height: 760
    minimumWidth: 760
    minimumHeight: 560
    visible: true
    title: showingLibrary
               ? qsTr("Library - SZHBooks")
               : readerController.title.length > 0
                 ? readerController.title + qsTr(" - SZHBooks")
                 : qsTr("SZHBooks")

    color: Theme.windowColor

    function openBook(fileUrl) {
        if (root.readerController.openFile(fileUrl)) {
            root.showingLibrary = false
        }
    }

    function addBook(fileUrl) {
        if (!root.oneDriveLibraryService.configured) {
            root.openBook(fileUrl)
            return
        }

        const selectedFolder = !root.showingLibrary
                             || root.libraryModel.collectionFilter === "."
                             ? ""
                             : root.libraryModel.collectionFilter
        const managedUrl = root.oneDriveLibraryService.importBook(fileUrl,
                                                                  selectedFolder)
        if (managedUrl.toString().length > 0) {
            root.openBook(managedUrl)
        }
    }

    function showLibrary() {
        root.exitFocusMode()
        readerWorkspace.flushReadingState()
        root.localStateStore.sync()
        root.libraryModel.refresh()
        root.showingLibrary = true
    }

    function toggleColorTheme() {
        root.localStateStore.darkMode = !root.localStateStore.darkMode
    }

    function enterFocusMode() {
        if (root.focusMode || root.showingLibrary || !readerWorkspace.hasDocument) {
            return
        }
        root.visibilityBeforeFocus = root.visibility === Window.Maximized
                                     ? Window.Maximized : Window.Windowed
        root.focusMode = true
        root.showFullScreen()
    }

    function exitFocusMode() {
        if (!root.focusMode) {
            return
        }
        root.focusMode = false
        if (root.visibilityBeforeFocus === Window.Maximized) {
            root.showMaximized()
        } else {
            root.showNormal()
        }
    }

    function toggleFocusMode() {
        if (root.focusMode) {
            root.exitFocusMode()
        } else {
            root.enterFocusMode()
        }
    }

    function locateBook(sourceUrl) {
        root.libraryModel.clearError()
        root.relinkSourceUrl = sourceUrl
        relinkDialog.open()
    }

    function flushLocalProfile() {
        readerWorkspace.flushReadingState()
        root.readingAnnotationStore.sync()
        root.localStateStore.sync()
    }

    function showProfileNotice(heading, message, kind) {
        root.profileNoticeHeading = heading
        root.profileNoticeMessage = message
        root.profileNoticeKind = kind || "info"
        root.profileNoticeShown = true
    }

    onClosing: {
        readerWorkspace.flushReadingState()
        root.localStateStore.sync()
    }

    Binding {
        target: Theme
        property: "colorTheme"
        value: root.localStateStore.colorTheme
    }

    AppShortcuts {
        showingLibrary: root.showingLibrary
        focusMode: root.focusMode
        readingNavigationEnabled: root.readingNavigationEnabled
        readerWorkspace: readerWorkspace
        appHeader: appHeader
        onOpenRequested: openDialog.open()
        onColorThemeToggleRequested: root.toggleColorTheme()
        onFocusModeToggleRequested: root.toggleFocusMode()
        onFocusModeExitRequested: root.exitFocusMode()
    }

    FileDialog {
        id: openDialog

        title: qsTr("Open book")
        nameFilters: root.supportedBookNameFilters

        onAccepted: root.addBook(selectedFile)
    }

    FolderDialog {
        id: libraryFolderDialog

        title: qsTr("Choose OneDrive library folder")
        currentFolder: root.oneDriveLibraryService.configured
                       ? root.oneDriveLibraryService.rootFolder
                       : root.oneDriveLibraryService.suggestedFolder
        onAccepted: {
            if (root.oneDriveLibraryService.setRootFolder(selectedFolder)) {
                root.libraryModel.collectionFilter = ""
            }
        }
    }

    CreateCollectionDialog {
        id: createCollectionDialog

        syncService: root.oneDriveLibraryService
        onCollectionCreated: collectionPath => {
            root.libraryModel.collectionFilter = collectionPath
            root.libraryModel.refresh()
        }
    }

    SyncCenterDialog {
        id: syncCenterDialog

        syncService: root.oneDriveLibraryService
    }

    FileDialog {
        id: relinkDialog

        title: qsTr("Locate moved book")
        nameFilters: root.supportedBookNameFilters
        onAccepted: {
            if (root.libraryModel.relinkBook(root.relinkSourceUrl, selectedFile))
                root.relinkSourceUrl = ""
        }
        onRejected: root.relinkSourceUrl = ""
    }

    FileDialog {
        id: profileBackupDialog

        title: qsTr("Back up local profile")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "szhbackup"
        nameFilters: [qsTr("SZHBooks profile (*.szhbackup)")]
        onAccepted: {
            root.flushLocalProfile()
            root.profileArchiveService.exportProfile(selectedFile)
        }
    }

    FileDialog {
        id: profileRestoreFileDialog

        title: qsTr("Restore local profile")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("SZHBooks profile (*.szhbackup)")]
        onAccepted: {
            root.pendingProfileRestoreUrl = selectedFile
            profileRestoreDialog.open()
        }
    }

    ProfileRestoreDialog {
        id: profileRestoreDialog

        backupUrl: root.pendingProfileRestoreUrl
        onRestoreConfirmed: {
            root.flushLocalProfile()
            root.profileArchiveService.importProfile(root.pendingProfileRestoreUrl)
        }
        onClosed: root.pendingProfileRestoreUrl = ""
    }

    Connections {
        target: root.profileArchiveService

        function onProfileExported() {
            root.showProfileNotice(qsTr("Backup saved"),
                                   qsTr("Settings, library, positions and notes were saved."),
                                   "success")
        }

        function onProfileImported() {
            if (readerWorkspace.hasDocument) {
                Qt.callLater(readerWorkspace.restoreReadingState)
            }
            root.showProfileNotice(qsTr("Profile restored"),
                                   qsTr("Your local reading data is ready."),
                                   "success")
        }

        function onOperationFailed(errorMessage) {
            root.showProfileNotice(qsTr("Profile operation failed"), errorMessage, "error")
        }
    }

    Connections {
        target: root.oneDriveLibraryService

        function onProfileApplied() {
            if (readerWorkspace.hasDocument) {
                Qt.callLater(readerWorkspace.restoreReadingState)
            }
        }

        function onConflictsDetected(conflictFile, count) {
            root.showProfileNotice(
                        qsTr("Synchronization conflict preserved"),
                        qsTr("%n conflicting value(s) were kept in %1.", "", count)
                            .arg(conflictFile.toString()),
                        "info")
        }

        function onOperationFailed(errorMessage) {
            root.showProfileNotice(qsTr("OneDrive synchronization"),
                                   errorMessage,
                                   "error")
        }
    }

    header: AppHeader {
        id: appHeader
        objectName: "appHeader"

        readerController: root.readerController
        readerWorkspace: readerWorkspace
        settingsStore: root.localStateStore
        localizationController: root.localizationController
        darkMode: root.localStateStore.darkMode
        showingLibrary: root.showingLibrary
        visible: !root.focusMode
        onOpenRequested: openDialog.open()
        onLibraryRequested: root.showLibrary()
        onDarkModeToggled: root.localStateStore.darkMode = darkMode
        onFocusModeRequested: root.toggleFocusMode()
        onBackupProfileRequested: profileBackupDialog.open()
        onRestoreProfileRequested: profileRestoreFileDialog.open()
    }

    footer: ReaderStatusBar {
        readerController: root.readerController
        readerWorkspace: readerWorkspace
        visible: !root.showingLibrary && !root.focusMode
    }

    ReaderWorkspace {
        id: readerWorkspace
        objectName: "readerWorkspace"

        anchors.fill: parent
        visible: !root.showingLibrary
        opacity: root.showingLibrary ? 0 : 1
        readerController: root.readerController
        settingsStore: root.localStateStore
        documentFormatter: root.readingDocumentFormatter
        searchController: root.readingSearchController
        annotationStore: root.readingAnnotationStore
        onOpenRequested: openDialog.open()

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionNormal
            }
        }
    }

    LibraryView {
        anchors.fill: parent
        visible: root.showingLibrary
        opacity: root.showingLibrary ? 1 : 0
        libraryModel: root.libraryModel
        syncService: root.oneDriveLibraryService
        onAddRequested: openDialog.open()
        onOpenRequested: sourceUrl => root.openBook(sourceUrl)
        onRelinkRequested: sourceUrl => root.locateBook(sourceUrl)
        onChooseFolderRequested: libraryFolderDialog.open()
        onCreateCollectionRequested: parentPath => {
            createCollectionDialog.openFor(parentPath)
        }
        onSyncDetailsRequested: syncCenterDialog.open()

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionNormal
            }
        }
    }

    SZHIconButton {
        z: 90
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        visible: root.focusMode && !root.showingLibrary
        opacity: focusExitHover.hovered || activeFocus ? 1 : 0.42
        symbol: "\u26f6"
        symbolPixelSize: Theme.bodyLargeFontSize
        toolTip: qsTr("Exit focus mode (F11)")
        onClicked: root.exitFocusMode()

        HoverHandler {
            id: focusExitHover
        }

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionFast
            }
        }
    }

    SZHNotification {
        id: readerErrorNotification

        z: 100
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        width: Math.min(480, Math.max(320, root.width - Theme.spaceXl))
        shown: root.readerController.errorMessage.length > 0
        heading: qsTr("Could not open book")
        message: root.readerController.errorMessage
        actionText: qsTr("Choose another")
        onActionRequested: openDialog.open()
        onDismissed: root.readerController.clearError()
    }

    SZHNotification {
        id: libraryErrorNotification

        z: 101
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceMd
                           + (readerErrorNotification.visible
                              ? readerErrorNotification.height + Theme.spaceSm
                              : 0)
        anchors.rightMargin: Theme.spaceMd
        width: Math.min(480, Math.max(320, root.width - Theme.spaceXl))
        shown: root.libraryModel.errorMessage.length > 0
        heading: qsTr("Could not update library")
        message: root.libraryModel.errorMessage
        actionText: qsTr("Choose another")
        onActionRequested: relinkDialog.open()
        onDismissed: root.libraryModel.clearError()
    }

    SZHNotification {
        z: 102
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.spaceMd
                           + (readerErrorNotification.visible
                              ? readerErrorNotification.height + Theme.spaceSm
                              : 0)
                           + (libraryErrorNotification.visible
                              ? libraryErrorNotification.height + Theme.spaceSm
                              : 0)
        anchors.rightMargin: Theme.spaceMd
        width: Math.min(480, Math.max(320, root.width - Theme.spaceXl))
        shown: root.profileNoticeShown
        kind: root.profileNoticeKind
        heading: root.profileNoticeHeading
        message: root.profileNoticeMessage
        onDismissed: root.profileNoticeShown = false
    }
}
