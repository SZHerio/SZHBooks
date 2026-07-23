pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var model: []
    property string value: ""

    signal valueSelected(string value)

    implicitWidth: 260
    implicitHeight: Theme.controlHeight
    color: Theme.surfaceMutedColor
    radius: Theme.radiusMd
    border.color: Theme.borderColor
    border.width: 1
    clip: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 2

        Repeater {
            model: root.model

            delegate: Button {
                id: optionButton

                required property var modelData

                objectName: "segment_" + optionButton.modelData.value
                Layout.fillWidth: true
                Layout.fillHeight: true
                padding: 0
                checkable: true
                checked: root.value === optionButton.modelData.value
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                Accessible.role: Accessible.RadioButton
                Accessible.name: optionButton.modelData.label
                Accessible.checkable: true
                Accessible.checked: optionButton.checked
                onClicked: root.valueSelected(optionButton.modelData.value)

                contentItem: Text {
                    text: optionButton.modelData.label
                    color: optionButton.checked || optionButton.hovered || optionButton.down
                           ? Theme.interactionTextColor : Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                    font.weight: optionButton.checked ? Font.DemiBold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: optionButton.checked || optionButton.hovered || optionButton.down
                           ? Theme.interactionColor : "transparent"
                    radius: Theme.radiusSm
                    border.color: optionButton.activeFocus
                                  ? Theme.focusColor
                                  : "transparent"
                    border.width: optionButton.activeFocus ? 2 : 0
                }
            }
        }
    }
}
