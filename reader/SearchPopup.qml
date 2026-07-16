import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var readerWorkspace

    function openAndFocus() {
        root.open()
        Qt.callLater(function() {
            searchField.forceActiveFocus()
            searchField.selectAll()
        })
    }

    width: Math.min(400,
                    Math.max(300,
                             (parent ? parent.width : 432) - Theme.spaceXl))
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

    contentItem: RowLayout {
        spacing: Theme.spaceXs

        SZHTextField {
            id: searchField

            Layout.fillWidth: true
            placeholderText: qsTr("Search in book")
            text: root.readerWorkspace.searchQuery
            onTextEdited: root.readerWorkspace.searchQuery = text
            Keys.onReturnPressed: event => {
                if (event.modifiers & Qt.ShiftModifier) {
                    root.readerWorkspace.previousSearchResult()
                } else {
                    root.readerWorkspace.nextSearchResult()
                }
                event.accepted = true
            }
        }

        Label {
            Layout.preferredWidth: 58
            text: root.readerWorkspace.searchQuery.trim().length === 0
                  ? ""
                  : root.readerWorkspace.searchResultCount === 0
                    ? qsTr("0 of 0")
                    : qsTr("%1 of %2")
                        .arg(root.readerWorkspace.currentSearchIndex + 1)
                        .arg(root.readerWorkspace.searchResultCount)
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            horizontalAlignment: Text.AlignHCenter
        }

        SZHIconButton {
            symbol: "\u2191"
            toolTip: qsTr("Previous result")
            enabled: root.readerWorkspace.searchResultCount > 0
            onClicked: root.readerWorkspace.previousSearchResult()
        }

        SZHIconButton {
            symbol: "\u2193"
            toolTip: qsTr("Next result")
            enabled: root.readerWorkspace.searchResultCount > 0
            onClicked: root.readerWorkspace.nextSearchResult()
        }

        SZHIconButton {
            symbol: "\u00d7"
            toolTip: qsTr("Close search")
            onClicked: root.close()
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: Theme.motionFast
            }
            NumberAnimation {
                property: "scale"
                from: 0.98
                to: 1
                duration: Theme.motionFast
            }
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
