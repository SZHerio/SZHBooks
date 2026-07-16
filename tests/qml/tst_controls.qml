import QtQuick
import QtQuick.Controls
import QtTest
import SZHBooksUiTest

TestCase {
    id: testCase

    name: "KeyboardAndFocus"
    when: windowShown

    Item {
        id: stage

        width: 640
        height: 480
    }

    Component {
        id: buttonComponent

        SZHButton {
            text: "Open book"
        }
    }

    Component {
        id: segmentedComponent

        SZHSegmentedControl {
            width: 320
            model: [
                { value: "light", label: "Light" },
                { value: "dark", label: "Dark" }
            ]
            value: "light"
        }
    }

    Component {
        id: sliderComponent

        SZHSlider {
            accessibleName: "Scroll speed"
            from: 50
            to: 200
            stepSize: 10
            value: 100
        }
    }

    Component {
        id: focusChainComponent

        Item {
            width: 320
            height: 100

            SZHButton {
                id: firstControl

                objectName: "firstControl"
                text: "First"
            }

            SZHTextField {
                id: secondControl

                objectName: "secondControl"
                anchors.top: firstControl.bottom
                placeholderText: "Second"
            }
        }
    }

    Component {
        id: restoreDialogComponent

        ProfileRestoreDialog {
            backupUrl: "file:///C:/Books/profile.szhbackup"
        }
    }

    Component {
        id: bookDelegateComponent

        LibraryBookDelegate {
            width: 188
            height: 288
            index: 0
            sourceUrl: "file:///C:/Books/novel.txt"
            sourcePath: "C:/Books/novel.txt"
            title: "Novel"
            author: "Author"
            formatName: "TXT"
            coverUrl: ""
            progress: 0.42
            lastOpened: new Date(2026, 0, 1)
            fileAvailable: true
        }
    }

    QtObject {
        id: shortcutWorkspace

        property bool hasDocument: true
        property int forwardCount: 0
        property int backwardCount: 0
        function increaseScale() {}
        function decreaseScale() {}
        function toggleCurrentBookmark() {}
        function pageForward() { forwardCount += 1 }
        function pageBackward() { backwardCount += 1 }
        function goToStart() {}
        function goToEnd() {}
    }

    QtObject {
        id: shortcutHeader

        function openSearch() {}
        function openAnnotations(tab) {}
    }

    Component {
        id: shortcutsComponent

        AppShortcuts {
            readerWorkspace: shortcutWorkspace
            appHeader: shortcutHeader
            showingLibrary: false
            readingNavigationEnabled: true
        }
    }

    function test_buttonActivatesFromKeyboard() {
        const button = createTemporaryObject(buttonComponent, stage)
        verify(button)

        let clickCount = 0
        button.clicked.connect(function() { clickCount += 1 })
        button.forceActiveFocus()
        tryCompare(button, "activeFocus", true)
        keyClick(Qt.Key_Space)
        compare(clickCount, 1)
    }

    function test_segmentAndSliderUseKeyboard() {
        const segmented = createTemporaryObject(segmentedComponent, stage)
        verify(segmented)
        const darkOption = findChild(segmented, "segment_dark")
        verify(darkOption)

        let selectedValue = ""
        segmented.valueSelected.connect(function(value) { selectedValue = value })
        darkOption.forceActiveFocus()
        tryCompare(darkOption, "activeFocus", true)
        keyClick(Qt.Key_Space)
        compare(selectedValue, "dark")

        const slider = createTemporaryObject(sliderComponent, stage)
        verify(slider)
        slider.forceActiveFocus()
        tryCompare(slider, "activeFocus", true)
        keyClick(Qt.Key_Right)
        compare(slider.value, 110)
    }

    function test_controlsAcceptTabFocus() {
        const chain = createTemporaryObject(focusChainComponent, stage)
        verify(chain)
        const first = findChild(chain, "firstControl")
        const second = findChild(chain, "secondControl")
        verify(first)
        verify(second)

        first.forceActiveFocus()
        tryCompare(first, "activeFocus", true)
        verify(first.activeFocusOnTab)
        verify(second.activeFocusOnTab)
        second.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(second, "activeFocus", true)
    }

    function test_restoreDialogStartsSafelyAndClosesWithEscape() {
        const dialog = createTemporaryObject(restoreDialogComponent, stage)
        verify(dialog)
        dialog.open()
        tryCompare(dialog, "opened", true)

        const cancel = findChild(dialog, "cancelRestoreButton")
        verify(cancel)
        tryCompare(cancel, "activeFocus", true)
        keyClick(Qt.Key_Escape)
        tryCompare(dialog, "opened", false)
    }

    function test_libraryBookOpensFromKeyboard() {
        const delegate = createTemporaryObject(bookDelegateComponent, stage)
        verify(delegate)

        let openedUrl = ""
        delegate.openRequested.connect(function(sourceUrl) {
            openedUrl = sourceUrl.toString()
        })
        delegate.forceActiveFocus()
        tryCompare(delegate, "activeFocus", true)
        keyClick(Qt.Key_Return)
        compare(openedUrl, "file:///C:/Books/novel.txt")
    }

    function test_applicationShortcutsRouteCommands() {
        const shortcuts = createTemporaryObject(shortcutsComponent, stage)
        verify(shortcuts)

        let openCount = 0
        let themeCount = 0
        shortcuts.openRequested.connect(function() { openCount += 1 })
        shortcuts.colorThemeToggleRequested.connect(function() { themeCount += 1 })
        shortcutWorkspace.forwardCount = 0

        keyClick(Qt.Key_O, Qt.ControlModifier)
        keyClick(Qt.Key_D, Qt.ControlModifier | Qt.ShiftModifier)
        keyClick(Qt.Key_PageDown)

        compare(openCount, 1)
        compare(themeCount, 1)
        compare(shortcutWorkspace.forwardCount, 1)
    }
}
