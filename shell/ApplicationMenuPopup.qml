import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var syncService
    property bool showingLibrary: false
    property bool libraryHasBooks: false
    property bool selectionMode: false

    signal openRequested
    signal libraryRequested
    signal librarySearchRequested
    signal notesCenterRequested
    signal selectionModeRequested
    signal createCollectionRequested
    signal chooseFolderRequested
    signal syncDetailsRequested
    signal settingsRequested
    signal aboutRequested

    width: 286
    padding: Theme.space2xs
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true

    function trigger(action) {
        root.close()
        action()
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: Theme.strongBorderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        SZHButton {
            id: firstMenuButton

            visible: !root.showingLibrary
            Layout.fillWidth: true
            text: qsTr("Library")
            symbol: "\u25a6"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.libraryRequested)
        }

        SZHButton {
            objectName: "applicationMenuAddBook"
            Layout.fillWidth: true
            text: qsTr("Add book")
            symbol: "+"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.openRequested)
        }

        SZHButton {
            visible: root.showingLibrary
            Layout.fillWidth: true
            text: qsTr("Search library")
            symbol: "\u2315"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.librarySearchRequested)
        }

        SZHButton {
            visible: root.showingLibrary
            Layout.fillWidth: true
            text: qsTr("Notes center")
            symbol: "\u2261"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.notesCenterRequested)
        }

        SZHButton {
            visible: root.showingLibrary && root.libraryHasBooks
            Layout.fillWidth: true
            text: root.selectionMode ? qsTr("Finish selecting") : qsTr("Select books")
            symbol: root.selectionMode ? "\u00d7" : "\u2713"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.selectionModeRequested)
        }

        Rectangle {
            visible: root.showingLibrary
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXs
            Layout.rightMargin: Theme.spaceXs
            Layout.topMargin: Theme.space2xs
            Layout.bottomMargin: Theme.space2xs
            implicitHeight: 1
            color: Theme.borderColor
        }

        SZHButton {
            visible: root.showingLibrary && root.syncService.configured
            Layout.fillWidth: true
            text: qsTr("New folder")
            symbol: "+"
            variant: "flat"
            enabled: root.syncService.available && !root.syncService.fileService.busy
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.createCollectionRequested)
        }

        SZHButton {
            visible: root.showingLibrary
            objectName: "applicationMenuOneDriveFolder"
            Layout.fillWidth: true
            text: qsTr("OneDrive library folder")
            symbol: "\u2601"
            variant: "flat"
            enabled: !root.syncService.fileService.busy
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.chooseFolderRequested)
        }

        SZHButton {
            visible: root.showingLibrary && root.syncService.configured
            Layout.fillWidth: true
            text: qsTr("Synchronization details")
            symbol: "\u21bb"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.syncDetailsRequested)
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXs
            Layout.rightMargin: Theme.spaceXs
            Layout.topMargin: Theme.space2xs
            Layout.bottomMargin: Theme.space2xs
            implicitHeight: 1
            color: Theme.borderColor
        }

        SZHButton {
            objectName: "applicationMenuSettings"
            Layout.fillWidth: true
            text: qsTr("Settings")
            symbol: "\u2699"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.settingsRequested)
        }

        SZHButton {
            Layout.fillWidth: true
            text: qsTr("About SZHBooks")
            symbol: "i"
            variant: "flat"
            Accessible.role: Accessible.MenuItem
            onClicked: root.trigger(root.aboutRequested)
        }
    }

    onOpened: {
        const firstVisibleButton = root.showingLibrary
                                 ? contentItem.children[1]
                                 : firstMenuButton
        if (firstVisibleButton)
            firstVisibleButton.forceActiveFocus()
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.motionFast
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
