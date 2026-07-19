import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property int index
    required property url sourceUrl
    required property string sourcePath
    required property string title
    required property string author
    required property string formatName
    required property string collectionPath
    required property url coverUrl
    required property real progress
    required property date lastOpened
    required property bool fileAvailable

    property color fallbackColor: Theme.accentColor

    signal openRequested(url sourceUrl)
    signal removeRequested(int row)
    signal relinkRequested(url sourceUrl)

    function activate() {
        if (root.fileAvailable) {
            root.openRequested(root.sourceUrl)
        } else {
            root.relinkRequested(root.sourceUrl)
        }
    }

    activeFocusOnTab: true
    Accessible.role: Accessible.ListItem
    Accessible.name: root.title
    Accessible.description: root.fileAvailable
                                ? qsTr("%1, %2% read")
                                    .arg(root.author.length > 0
                                         ? root.author : root.formatName)
                                    .arg(Math.round(root.progress * 100))
                                : qsTr("File unavailable. Locate the book to continue.")
    Accessible.focusable: true
    Accessible.onPressAction: root.activate()
    opacity: fileAvailable ? 1 : 0.72
    scale: cardMouseArea.containsMouse ? 1.008 : 1

    Behavior on scale {
        NumberAnimation {
            duration: Theme.motionFast
            easing.type: Easing.OutCubic
        }
    }

    Keys.onReturnPressed: root.activate()
    Keys.onEnterPressed: root.activate()
    Keys.onSpacePressed: root.activate()

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        color: cardMouseArea.containsMouse ? Theme.surfaceMutedColor : Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: root.activeFocus ? Theme.focusColor : Theme.borderColor
        border.width: root.activeFocus ? 2 : 1

        Behavior on color {
            ColorAnimation {
                duration: Theme.motionFast
            }
        }
    }

    MouseArea {
        id: cardMouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.forceActiveFocus()
            root.activate()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        spacing: Theme.spaceXs

        LibraryBookCover {
            Layout.fillWidth: true
            Layout.preferredHeight: 158
            title: root.title
            formatName: root.formatName
            coverUrl: root.coverUrl
            fallbackColor: root.fallbackColor
        }

        Label {
            Layout.fillWidth: true
            text: root.title
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyFontSize
            font.weight: Font.DemiBold
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            text: root.fileAvailable
                  ? ([root.author, root.collectionPath,
                      Qt.formatDateTime(root.lastOpened, qsTr("dd MMM, HH:mm"))]
                     .filter(value => value.length > 0).join(qsTr("  \u00b7  ")))
                  : qsTr("File unavailable")
            color: root.fileAvailable ? Theme.mutedTextColor : Theme.dangerColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            elide: Text.ElideRight
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            visible: root.fileAvailable
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Label {
                text: Math.round(root.progress * 100) + qsTr("%")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
                font.weight: Font.DemiBold
            }

            SZHProgressBar {
                Layout.fillWidth: true
                value: root.progress
            }
        }

        SZHButton {
            visible: !root.fileAvailable
            Layout.fillWidth: true
            variant: "secondary"
            text: qsTr("Locate file")
            onClicked: root.relinkRequested(root.sourceUrl)
        }
    }

    SZHIconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Theme.spaceSm
        z: 2
        visible: cardMouseArea.containsMouse || root.activeFocus
        symbol: "\u00d7"
        toolTip: qsTr("Remove from library")
        onClicked: root.removeRequested(root.index)
    }

    ToolTip {
        visible: cardMouseArea.containsMouse && !root.fileAvailable
        text: root.sourcePath
        delay: Theme.tooltipDelay
    }
}
