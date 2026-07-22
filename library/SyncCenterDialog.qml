import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    required property var syncService
    property string currentView: "status"

    function statusHeading() {
        if (!root.syncService.configured)
            return qsTr("OneDrive is not configured")
        if (root.syncService.syncing)
            return qsTr("Synchronizing library")
        if (!root.syncService.available)
            return qsTr("Library folder is unavailable")
        if (root.syncService.conflictCount > 0)
            return qsTr("Conflicts need your choice")
        if (root.syncService.status === "error")
            return qsTr("Synchronization needs attention")
        return qsTr("Library is up to date")
    }

    function activityTitle(event) {
        const titles = {
            "folderConfigured": qsTr("Library folder configured"),
            "folderAvailable": qsTr("Library folder available again"),
            "folderUnavailable": qsTr("Library folder unavailable"),
            "syncStarted": qsTr("Synchronization started"),
            "syncCompleted": qsTr("Synchronization completed"),
            "syncFailed": qsTr("Synchronization failed"),
            "conflictsDetected": qsTr("Conflicts detected"),
            "conflictResolved": qsTr("Conflict resolved"),
            "booksImported": qsTr("Books imported"),
            "bookMoved": qsTr("Book moved"),
            "bookRenamed": qsTr("Book renamed"),
            "bookRemoved": qsTr("Book removed"),
            "collectionCreated": qsTr("Folder created"),
            "collectionRenamed": qsTr("Folder renamed"),
            "collectionRemoved": qsTr("Folder removed")
        }
        return titles[event] || qsTr("Library updated")
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - Theme.space2xl : 720)
    height: Math.min(600, parent ? parent.height - Theme.space2xl : 600)
    modal: true
    closePolicy: Popup.CloseOnEscape
    padding: 0

    background: Rectangle {
        color: Theme.surfaceColor
        border.color: Theme.borderColor
        border.width: 1
        radius: Theme.radiusLg
    }

    header: Item {
        implicitHeight: 64

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spaceLg
            anchors.rightMargin: Theme.spaceMd
            spacing: Theme.spaceMd

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2xs

                Label {
                    text: qsTr("Synchronization")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.titleFontSize
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.syncService.rootDisplayPath
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                    elide: Text.ElideMiddle
                }
            }

            SZHIconButton {
                symbol: "\u00d7"
                toolTip: qsTr("Close")
                onClicked: root.close()
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.borderColor
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            SZHSegmentedControl {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.spaceLg
                width: Math.min(480, parent.width - Theme.space2xl)
                model: [
                    {"value": "status", "label": qsTr("Status")},
                    {"value": "conflicts", "label": qsTr("Conflicts")},
                    {"value": "activity", "label": qsTr("Activity")}
                ]
                value: root.currentView
                onValueSelected: value => root.currentView = value
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentView === "conflicts" ? 1
                          : root.currentView === "activity" ? 2 : 0

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceLg
                    spacing: Theme.spaceLg

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceXs

                        Label {
                            Layout.fillWidth: true
                            text: root.statusHeading()
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyLargeFontSize
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.syncService.lastSyncedAt.getTime() > 0
                            text: qsTr("Last synchronized %1")
                                .arg(Qt.formatDateTime(root.syncService.lastSyncedAt,
                                                       qsTr("dd MMM, HH:mm")))
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.syncService.retryScheduled
                            text: qsTr("Automatic retry at %1")
                                .arg(Qt.formatTime(root.syncService.nextRetryAt,
                                                   qsTr("HH:mm:ss")))
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.syncService.errorMessage.length > 0
                            text: root.syncService.errorMessage
                            color: Theme.dangerColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            wrapMode: Text.Wrap
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.borderColor
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: Theme.spaceLg
                        rowSpacing: Theme.spaceSm

                        Label {
                            text: qsTr("Managed books")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Stored inside the selected OneDrive folder")
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            horizontalAlignment: Text.AlignRight
                            wrapMode: Text.Wrap
                        }
                        Label {
                            text: qsTr("Pending conflicts")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }
                        Label {
                            Layout.fillWidth: true
                            text: root.syncService.conflictCount
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignRight
                        }
                        Label {
                            text: qsTr("Online-only books")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }
                        Label {
                            Layout.fillWidth: true
                            text: root.syncService.cloudPlaceholderCount
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceSm

                        SZHButton {
                            text: qsTr("Open folder")
                            variant: "secondary"
                            enabled: root.syncService.available
                            onClicked: root.syncService.openRootFolder()
                        }
                        SZHButton {
                            visible: root.syncService.conflictCount > 0
                            text: qsTr("Review conflicts")
                            variant: "secondary"
                            onClicked: root.currentView = "conflicts"
                        }
                        Item { Layout.fillWidth: true }
                        SZHButton {
                            text: root.syncService.retryScheduled
                                  ? qsTr("Retry now") : qsTr("Synchronize now")
                            enabled: root.syncService.configured
                                     && !root.syncService.syncing
                            onClicked: root.syncService.retryNow()
                        }
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceLg
                    anchors.rightMargin: Theme.spaceLg
                    anchors.bottomMargin: Theme.spaceLg
                    spacing: Theme.spaceSm

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceSm

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Choose the value SZHBooks should keep")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                        }
                        SZHButton {
                            visible: root.syncService.conflictCount > 0
                            text: qsTr("Keep all on this device")
                            variant: "secondary"
                            onClicked: root.syncService.resolveAllConflicts("local")
                        }
                        SZHButton {
                            visible: root.syncService.conflictCount > 0
                            text: qsTr("Use all from OneDrive")
                            variant: "secondary"
                            onClicked: root.syncService.resolveAllConflicts("remote")
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.spaceXs
                        model: root.syncService.conflictModel
                        ScrollBar.vertical: SZHScrollBar {}

                        delegate: Rectangle {
                            required property string conflictId
                            required property string settingKey
                            required property string localValue
                            required property string remoteValue
                            required property date createdAt

                            width: ListView.view.width
                            height: conflictLayout.implicitHeight + Theme.spaceMd * 2
                            color: "transparent"
                            border.color: Theme.borderColor
                            border.width: 1
                            radius: Theme.radiusMd

                            ColumnLayout {
                                id: conflictLayout
                                anchors.fill: parent
                                anchors.margins: Theme.spaceMd
                                spacing: Theme.spaceSm

                                Label {
                                    Layout.fillWidth: true
                                    text: settingKey
                                    color: Theme.textColor
                                    font.family: Theme.uiFontFamily
                                    font.pixelSize: Theme.bodyFontSize
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideMiddle
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    columnSpacing: Theme.spaceSm
                                    rowSpacing: Theme.spaceXs

                                    Label {
                                        text: qsTr("This device")
                                        color: Theme.mutedTextColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.captionFontSize
                                    }
                                    Label {
                                        text: qsTr("OneDrive")
                                        color: Theme.mutedTextColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.captionFontSize
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: localValue
                                        color: Theme.textColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.bodyFontSize
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: remoteValue
                                        color: Theme.textColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.bodyFontSize
                                        elide: Text.ElideRight
                                    }
                                    SZHButton {
                                        Layout.fillWidth: true
                                        text: qsTr("Keep this device")
                                        variant: "secondary"
                                        onClicked: root.syncService.resolveConflict(
                                                       conflictId, "local")
                                    }
                                    SZHButton {
                                        Layout.fillWidth: true
                                        text: qsTr("Use OneDrive value")
                                        variant: "secondary"
                                        onClicked: root.syncService.resolveConflict(
                                                       conflictId, "remote")
                                    }
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: root.syncService.conflictCount === 0
                            text: qsTr("No unresolved conflicts")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyLargeFontSize
                        }
                    }

                    SZHButton {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Open conflict files")
                        variant: "secondary"
                        onClicked: root.syncService.openConflictsFolder()
                    }
                }
            }

            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceLg
                    anchors.rightMargin: Theme.spaceLg
                    anchors.bottomMargin: Theme.spaceLg
                    spacing: Theme.spaceSm

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.syncService.activityModel
                        ScrollBar.vertical: SZHScrollBar {}

                        delegate: Item {
                            required property string event
                            required property string severity
                            required property string detail
                            required property date timestamp

                            width: ListView.view.width
                            height: activityLayout.implicitHeight + Theme.spaceMd

                            RowLayout {
                                id: activityLayout
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Theme.spaceSm

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.space2xs

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.activityTitle(event)
                                        color: severity === "error"
                                               ? Theme.dangerColor : Theme.textColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.bodyFontSize
                                        font.weight: Font.DemiBold
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        visible: detail.length > 0
                                        text: detail
                                        color: Theme.mutedTextColor
                                        font.family: Theme.uiFontFamily
                                        font.pixelSize: Theme.captionFontSize
                                        elide: Text.ElideMiddle
                                    }
                                }

                                Label {
                                    text: Qt.formatDateTime(timestamp, qsTr("dd MMM, HH:mm"))
                                    color: Theme.mutedTextColor
                                    font.family: Theme.uiFontFamily
                                    font.pixelSize: Theme.captionFontSize
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: Theme.borderColor
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: root.syncService.activityModel.count === 0
                            text: qsTr("No synchronization activity yet")
                            color: Theme.mutedTextColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyLargeFontSize
                        }
                    }

                    SZHButton {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Clear activity")
                        variant: "secondary"
                        enabled: root.syncService.activityModel.count > 0
                        onClicked: root.syncService.activityModel.clear()
                    }
                }
            }
        }
    }
}
