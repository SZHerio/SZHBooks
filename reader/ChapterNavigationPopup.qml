pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var readerWorkspace

    width: 320
    height: Math.min(420, 66 + root.readerWorkspace.chapters.length * 44)
    padding: Theme.spaceSm
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: Theme.borderColor
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceXs

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceXs
            Layout.rightMargin: Theme.spaceXs
            text: qsTr("Chapters")
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.bodyLargeFontSize
            font.weight: Font.DemiBold
        }

        ListView {
            id: chapterList

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: root.readerWorkspace.currentChapterIndex
            model: root.readerWorkspace.chapters
            ScrollBar.vertical: SZHScrollBar {
            }

            delegate: Item {
                id: chapterRow

                required property int index
                required property var modelData

                width: chapterList.width
                height: 44

                Rectangle {
                    anchors.fill: parent
                    color: chapterMouseArea.containsMouse
                           ? Theme.surfaceMutedColor
                           : chapterRow.index === root.readerWorkspace.currentChapterIndex
                             ? Theme.accentSoftColor
                             : "transparent"
                    radius: Theme.radiusSm
                }

                MouseArea {
                    id: chapterMouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.readerWorkspace.goToChapter(chapterRow.index)
                        root.close()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceSm
                    anchors.rightMargin: Theme.spaceSm
                    spacing: Theme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: chapterRow.modelData.title
                        color: Theme.textColor
                        font.family: Theme.uiFontFamily
                        font.pixelSize: Theme.bodyFontSize
                        font.weight: chapterRow.index === root.readerWorkspace.currentChapterIndex
                                     ? Font.DemiBold
                                     : Font.Normal
                        elide: Text.ElideRight
                    }

                    Label {
                        text: Math.round(chapterRow.modelData.progress * 100) + qsTr("%")
                        color: Theme.mutedTextColor
                        font.family: Theme.uiFontFamily
                        font.pixelSize: Theme.captionFontSize
                    }
                }
            }
        }
    }
}
