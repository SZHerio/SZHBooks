import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var readerController
    required property var readerWorkspace

    readonly property bool showProgress: readerController.hasDocument

    implicitHeight: Theme.statusBarHeight
    color: Theme.surfaceColor
    border.color: Theme.borderColor
    border.width: 1

    Behavior on color {
        ColorAnimation {
            duration: Theme.motionNormal
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceLg
        anchors.rightMargin: Theme.spaceLg
        spacing: Theme.spaceSm

        Label {
            visible: root.showProgress
            text: root.readerController.formatName
            color: Theme.accentColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: root.readerWorkspace.currentChapterTitle.length > 0
                        ? root.readerWorkspace.currentChapterTitle
                        : root.readerController.sourcePath.length > 0
                          ? root.readerController.sourcePath
                          : qsTr("No file open")
            color: Theme.mutedTextColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            elide: Text.ElideMiddle
            verticalAlignment: Text.AlignVCenter
        }

        RowLayout {
            visible: root.showProgress
            spacing: Theme.space2xs

            SZHIconButton {
                symbol: "\u2190"
                toolTip: qsTr("Back to previous location (Alt+Left)")
                enabled: root.readerWorkspace.canNavigateBack
                onClicked: root.readerWorkspace.navigateBack()
            }

            SZHIconButton {
                symbol: "\u2192"
                toolTip: qsTr("Forward to next location (Alt+Right)")
                enabled: root.readerWorkspace.canNavigateForward
                onClicked: root.readerWorkspace.navigateForward()
            }
        }

        TextNavigationControl {
            visible: root.showProgress
                     && root.readerWorkspace.showingText
                     && !root.readerWorkspace.textPagedMode
            readerWorkspace: root.readerWorkspace
            compact: root.width < 900
        }

        PageNavigationControl {
            visible: root.showProgress
                     && (root.readerWorkspace.showingPdf
                         || root.readerWorkspace.textPagedMode)
            readerWorkspace: root.readerWorkspace
            compact: root.width < 980
        }
    }
}
