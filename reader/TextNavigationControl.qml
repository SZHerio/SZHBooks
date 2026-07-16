import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property var readerWorkspace
    property bool compact: false

    spacing: Theme.spaceXs

    LeaflitIconButton {
        symbol: "\u21e4"
        toolTip: qsTr("Go to beginning")
        enabled: root.readerWorkspace.canGoBackward
        onClicked: root.readerWorkspace.goToStart()
    }

    LeaflitSlider {
        Layout.preferredWidth: root.compact ? 110 : 190
        from: 0
        to: 1
        stepSize: 0.001
        value: root.readerWorkspace.readingProgress
        onMoved: root.readerWorkspace.goToProgress(value)
    }

    Label {
        Layout.preferredWidth: 42
        text: Math.round(root.readerWorkspace.readingProgress * 100) + qsTr("%")
        color: Theme.textColor
        font.family: Theme.uiFontFamily
        font.pixelSize: Theme.captionFontSize
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
    }

    LeaflitIconButton {
        symbol: "\u21e5"
        toolTip: qsTr("Go to end")
        enabled: root.readerWorkspace.canGoForward
        onClicked: root.readerWorkspace.goToEnd()
    }
}
