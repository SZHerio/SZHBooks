import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string symbol: ""
    property string toolTip: ""
    property bool onChrome: false
    property bool selected: false
    property int symbolPixelSize: Theme.iconFontSize
    readonly property bool interactionActive: enabled && (selected || down || hovered)

    readonly property color foregroundColor: !enabled
                                                 ? onChrome
                                                   ? Theme.chromeMutedTextColor
                                                   : Theme.disabledTextColor
                                                 : interactionActive
                                                   ? Theme.interactionTextColor
                                                   : onChrome
                                                     ? Theme.chromeTextColor
                                                     : Theme.textColor
    readonly property color backgroundColor: {
        if (interactionActive) {
            return Theme.interactionColor
        }
        return "transparent"
    }

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    Accessible.name: control.toolTip
    Accessible.role: Accessible.Button

    ToolTip.text: toolTip
    ToolTip.visible: hovered && toolTip.length > 0
    ToolTip.delay: Theme.tooltipDelay

    contentItem: Text {
        text: control.symbol
        color: control.foregroundColor
        font.family: Theme.uiFontFamily
        font.pixelSize: control.symbolPixelSize
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: control.backgroundColor
        radius: Theme.radiusMd
        border.color: control.activeFocus ? Theme.focusColor : "transparent"
        border.width: control.activeFocus ? 2 : 0

    }
}
