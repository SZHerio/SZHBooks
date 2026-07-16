import QtQuick

Item {
    id: root

    required property var readerWorkspace
    required property var appHeader
    property bool showingLibrary: true
    property bool focusMode: false
    property bool readingNavigationEnabled: false

    signal openRequested
    signal colorThemeToggleRequested
    signal focusModeToggleRequested
    signal focusModeExitRequested

    visible: false

    Shortcut {
        sequence: StandardKey.Open
        onActivated: root.openRequested()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: StandardKey.Find
        onActivated: root.appHeader.openSearch()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: StandardKey.ZoomIn
        onActivated: root.readerWorkspace.increaseScale()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: StandardKey.ZoomOut
        onActivated: root.readerWorkspace.decreaseScale()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: "Ctrl+B"
        onActivated: root.readerWorkspace.toggleCurrentBookmark()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: "Ctrl+Shift+H"
        onActivated: root.appHeader.openAnnotations("highlights")
    }

    Shortcut {
        sequence: "Ctrl+Shift+D"
        onActivated: root.colorThemeToggleRequested()
    }

    Shortcut {
        enabled: !root.showingLibrary && root.readerWorkspace.hasDocument
        sequence: "F11"
        onActivated: root.focusModeToggleRequested()
    }

    Shortcut {
        enabled: root.focusMode
        sequence: "Esc"
        onActivated: root.focusModeExitRequested()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "PgUp"
        onActivated: root.readerWorkspace.pageBackward()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "PgDown"
        onActivated: root.readerWorkspace.pageForward()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "Home"
        onActivated: root.readerWorkspace.goToStart()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "End"
        onActivated: root.readerWorkspace.goToEnd()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "Space"
        onActivated: root.readerWorkspace.pageForward()
    }

    Shortcut {
        enabled: root.readingNavigationEnabled
        sequence: "Shift+Space"
        onActivated: root.readerWorkspace.pageBackward()
    }
}
