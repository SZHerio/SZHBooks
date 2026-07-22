import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var syncService

    signal chooseFolderRequested
    signal detailsRequested

    readonly property string statusText: {
        if (!root.syncService.configured) {
            return root.syncService.oneDriveDetected
                    ? qsTr("Suggested OneDrive folder")
                    : qsTr("Choose a folder inside OneDrive")
        }
        if (!root.syncService.available) {
            return qsTr("Folder unavailable")
        }
        if (root.syncService.syncing) {
            return qsTr("Synchronizing")
        }
        if (root.syncService.retryScheduled) {
            return qsTr("Retry scheduled for %1")
                    .arg(Qt.formatTime(root.syncService.nextRetryAt, qsTr("HH:mm:ss")))
        }
        if (root.syncService.status === "error") {
            return qsTr("Synchronization needs attention")
        }
        if (root.syncService.conflictCount > 0) {
            return qsTr("%n conflict(s) preserved", "", root.syncService.conflictCount)
        }
        if (root.syncService.status === "synced") {
            return qsTr("Up to date at %1")
                    .arg(Qt.formatTime(root.syncService.lastSyncedAt, qsTr("HH:mm")))
        }
        return qsTr("Ready to synchronize")
    }

    implicitHeight: contentLayout.implicitHeight + Theme.spaceSm * 2
    color: Theme.surfaceColor
    border.color: Theme.borderColor
    border.width: 1
    radius: Theme.radiusMd

    RowLayout {
        id: contentLayout

        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceSm
        anchors.topMargin: Theme.spaceSm
        anchors.bottomMargin: Theme.spaceSm
        spacing: Theme.spaceSm

        Label {
            text: "\u2601"
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.titleFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.preferredWidth: Theme.controlHeight
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 120
            spacing: Theme.space2xs

            Label {
                Layout.fillWidth: true
                text: root.syncService.configured
                      ? root.syncService.rootDisplayPath
                      : root.syncService.suggestedDisplayPath
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
            }

            Label {
                Layout.fillWidth: true
                text: root.statusText
                color: root.syncService.status === "error"
                       ? Theme.dangerColor
                       : Theme.mutedTextColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
                elide: Text.ElideRight
            }
        }

        SZHButton {
            objectName: "useSuggestedFolderButton"
            visible: !root.syncService.configured
            text: qsTr("Use this folder")
            enabled: root.syncService.suggestedFolder.toString().length > 0
                     && !root.syncService.fileService.busy
            onClicked: root.syncService.useSuggestedFolder()
        }

        SZHButton {
            objectName: "chooseLibraryFolderButton"
            text: root.syncService.configured ? qsTr("Change") : qsTr("Choose folder")
            variant: "secondary"
            enabled: !root.syncService.fileService.busy
            onClicked: root.chooseFolderRequested()
        }

        SZHIconButton {
            objectName: "synchronizeLibraryButton"
            visible: root.syncService.configured
            enabled: root.syncService.available && !root.syncService.syncing
            symbol: "\u21bb"
            toolTip: qsTr("Synchronize now")
            onClicked: root.syncService.synchronizeNow()
        }

        SZHIconButton {
            objectName: "syncDetailsButton"
            visible: root.syncService.configured
            symbol: "\u22ef"
            toolTip: qsTr("Synchronization details")
            onClicked: root.detailsRequested()
        }
    }
}
