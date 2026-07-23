import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string variant: "primary"
    property string symbol: ""
    readonly property bool interactionActive: enabled && (down || hovered)
    readonly property bool textTruncated: textLabel.truncated

    readonly property color resolvedForegroundColor: {
        if (!enabled) {
            return variant === "chrome" ? Theme.chromeMutedTextColor : Theme.disabledTextColor
        }
        if (interactionActive) {
            return Theme.interactionTextColor
        }
        if (variant === "primary") {
            return Theme.primaryActionTextColor
        }
        if (variant === "chrome") {
            return Theme.chromeTextColor
        }
        return Theme.textColor
    }

    readonly property color resolvedBackgroundColor: {
        if (variant === "primary") {
            if (!enabled) {
                return Theme.accentSoftColor
            }
            return interactionActive ? Theme.interactionColor : Theme.primaryActionColor
        }
        if (variant === "chrome") {
            return interactionActive ? Theme.interactionColor : "transparent"
        }
        if (variant === "secondary") {
            return interactionActive ? Theme.interactionColor : Theme.surfaceColor
        }
        return interactionActive ? Theme.interactionColor : "transparent"
    }

    readonly property color resolvedBorderColor: activeFocus
                                                    ? Theme.focusColor
                                                    : variant === "secondary"
                                                      ? Theme.borderColor
                                                      : "transparent"

    implicitWidth: Math.max(80, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceMd
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.spaceXs
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.Button
    Accessible.name: control.text
    palette.buttonText: resolvedForegroundColor

    font.family: Theme.uiFontFamily
    font.pixelSize: Theme.bodyFontSize
    font.weight: Font.DemiBold

    contentItem: Row {
        spacing: control.spacing

        Text {
            id: symbolLabel

            visible: control.symbol.length > 0
            anchors.verticalCenter: parent.verticalCenter
            text: control.symbol
            color: control.resolvedForegroundColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.iconFontSize
        }

        Text {
            id: textLabel

            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth,
                            Math.max(0,
                                     control.availableWidth
                                     - (symbolLabel.visible
                                        ? symbolLabel.implicitWidth + parent.spacing
                                        : 0)))
            text: control.text
            color: control.resolvedForegroundColor
            font: control.font
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        color: control.resolvedBackgroundColor
        radius: Theme.radiusMd
        border.color: control.resolvedBorderColor
        border.width: control.activeFocus ? 2 : control.variant === "secondary" ? 1 : 0

    }
}
