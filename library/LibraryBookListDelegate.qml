import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property int index
    required property url sourceUrl
    required property string sourcePath
    required property string title
    required property string author
    required property string formatName
    required property url coverUrl
    required property real progress
    required property date lastOpened
    required property bool fileAvailable

    property color fallbackColor: Theme.accentColor

    signal openRequested(url sourceUrl)
    signal removeRequested(int row)
    signal relinkRequested(url sourceUrl)

    activeFocusOnTab: true
    color: rowMouseArea.containsMouse ? Theme.surfaceMutedColor : Theme.surfaceColor
    radius: Theme.radiusMd
    border.color: activeFocus ? Theme.focusColor : Theme.borderColor
    border.width: activeFocus ? 2 : 1
    opacity: fileAvailable ? 1 : 0.72

    Behavior on color {
        ColorAnimation {
            duration: Theme.motionFast
        }
    }

    Keys.onReturnPressed: {
        if (root.fileAvailable) {
            root.openRequested(root.sourceUrl)
        } else {
            root.relinkRequested(root.sourceUrl)
        }
    }
    Keys.onEnterPressed: {
        if (root.fileAvailable) {
            root.openRequested(root.sourceUrl)
        } else {
            root.relinkRequested(root.sourceUrl)
        }
    }

    MouseArea {
        id: rowMouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.forceActiveFocus()
            if (root.fileAvailable) {
                root.openRequested(root.sourceUrl)
            } else {
                root.relinkRequested(root.sourceUrl)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXs
        spacing: Theme.spaceSm

        LibraryBookCover {
            Layout.preferredWidth: 46
            Layout.fillHeight: true
            title: root.title
            formatName: root.formatName
            coverUrl: root.coverUrl
            fallbackColor: root.fallbackColor
            compact: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space2xs

            Label {
                Layout.fillWidth: true
                text: root.title
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.bodyFontSize
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.fileAvailable
                      ? (root.author.length > 0
                         ? root.author
                         : Qt.formatDateTime(root.lastOpened, qsTr("dd MMM, HH:mm")))
                      : root.sourcePath
                color: root.fileAvailable ? Theme.mutedTextColor : Theme.dangerColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
                elide: Text.ElideMiddle
            }
        }

        Label {
            Layout.preferredWidth: 68
            text: root.formatName
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            visible: root.fileAvailable
            Layout.preferredWidth: 170
            spacing: Theme.spaceXs

            SZHProgressBar {
                Layout.fillWidth: true
                value: root.progress
            }

            Label {
                Layout.preferredWidth: 34
                text: Math.round(root.progress * 100) + qsTr("%")
                color: Theme.textColor
                font.family: Theme.uiFontFamily
                font.pixelSize: Theme.captionFontSize
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignRight
            }
        }

        SZHButton {
            visible: !root.fileAvailable
            Layout.preferredWidth: 84
            variant: "secondary"
            text: qsTr("Locate")
            onClicked: root.relinkRequested(root.sourceUrl)
        }

        SZHIconButton {
            symbol: "\u00d7"
            toolTip: qsTr("Remove from library")
            onClicked: root.removeRequested(root.index)
        }
    }
}
