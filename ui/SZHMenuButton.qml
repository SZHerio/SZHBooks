pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Button {
    id: control

    property var options: []
    property string value: ""
    property string labelPrefix: ""

    signal valueSelected(string value)

    function selectedLabel() {
        for (var index = 0; index < options.length; ++index) {
            if (options[index].value === value) {
                return options[index].label
            }
        }
        return options.length > 0 ? options[0].label : ""
    }

    readonly property string displayText: labelPrefix.length > 0
                                                 ? labelPrefix + qsTr(": ") + selectedLabel()
                                                 : selectedLabel()

    implicitWidth: 148
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spaceSm
    rightPadding: Theme.spaceSm
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    onClicked: optionMenu.open()

    contentItem: Row {
        spacing: Theme.spaceXs

        Text {
            width: Math.max(0, parent.width - menuArrow.width - parent.spacing)
            anchors.verticalCenter: parent.verticalCenter
            text: control.displayText
            color: Theme.textColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            font.weight: Font.Medium
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: menuArrow

            anchors.verticalCenter: parent.verticalCenter
            text: "\u25be"
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
        }
    }

    background: Rectangle {
        color: control.down
               ? Theme.accentSoftColor
               : control.hovered
                 ? Theme.surfaceMutedColor
                 : Theme.surfaceColor
        radius: Theme.radiusMd
        border.color: control.activeFocus ? Theme.focusColor : Theme.borderColor
        border.width: control.activeFocus ? 2 : 1
    }

    Popup {
        id: optionMenu

        y: control.height + Theme.space2xs
        width: Math.max(control.width, 180)
        padding: Theme.space2xs
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.radiusMd
            border.color: Theme.strongBorderColor
            border.width: 1
        }

        contentItem: Column {
            width: optionMenu.availableWidth

            Repeater {
                model: control.options

                delegate: Button {
                    id: optionButton

                    required property var modelData

                    width: parent.width
                    height: Theme.controlHeight
                    leftPadding: Theme.spaceSm
                    rightPadding: Theme.spaceSm
                    hoverEnabled: true
                    focusPolicy: Qt.StrongFocus
                    onClicked: {
                        control.valueSelected(optionButton.modelData.value)
                        optionMenu.close()
                    }

                    contentItem: Row {
                        spacing: Theme.spaceXs

                        Text {
                            width: Theme.iconFontSize
                            text: control.value === optionButton.modelData.value ? "\u2713" : ""
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            width: Math.max(0, parent.width - Theme.iconFontSize - parent.spacing)
                            text: optionButton.modelData.label
                            color: Theme.textColor
                            font.family: Theme.uiFontFamily
                            font.pixelSize: Theme.bodyFontSize
                            elide: Text.ElideRight
                        }
                    }

                    background: Rectangle {
                        color: optionButton.down
                               ? Theme.accentSoftColor
                               : optionButton.hovered
                                 ? Theme.surfaceMutedColor
                                 : "transparent"
                        radius: Theme.radiusSm
                        border.color: optionButton.activeFocus ? Theme.focusColor : "transparent"
                        border.width: optionButton.activeFocus ? 2 : 0
                    }
                }
            }
        }
    }
}
