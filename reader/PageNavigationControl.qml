import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property var readerWorkspace

    spacing: Theme.spaceXs

    function syncPageText() {
        if (!pageField.activeFocus) {
            pageField.text = root.readerWorkspace.currentPage >= 0
                             ? String(root.readerWorkspace.currentPage + 1)
                             : ""
        }
    }

    function commitPage() {
        const requestedPage = Number(pageField.text)
        if (Number.isInteger(requestedPage)
                && requestedPage >= 1
                && requestedPage <= root.readerWorkspace.pageCount) {
            root.readerWorkspace.goToPage(requestedPage - 1)
        }
        Qt.callLater(root.syncPageText)
    }

    Component.onCompleted: root.syncPageText()

    Connections {
        target: root.readerWorkspace

        function onCurrentPageChanged() {
            root.syncPageText()
        }

        function onPageCountChanged() {
            root.syncPageText()
        }
    }

    SZHIconButton {
        symbol: "\u2039"
        symbolPixelSize: Theme.titleFontSize
        toolTip: qsTr("Previous page")
        enabled: root.readerWorkspace.canGoBackward
        onClicked: root.readerWorkspace.previousPage()
    }

    SZHTextField {
        id: pageField

        Layout.preferredWidth: 48
        accessibleName: qsTr("Page number")
        horizontalAlignment: Text.AlignHCenter
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator {
            bottom: 1
            top: Math.max(1, root.readerWorkspace.pageCount)
        }
        onAccepted: {
            root.commitPage()
            focus = false
        }
        onEditingFinished: root.commitPage()
    }

    Label {
        text: qsTr("of %1").arg(root.readerWorkspace.pageCount)
        color: Theme.mutedTextColor
        font.family: Theme.uiFontFamily
        font.pixelSize: Theme.captionFontSize
        verticalAlignment: Text.AlignVCenter
    }

    SZHIconButton {
        symbol: "\u203a"
        symbolPixelSize: Theme.titleFontSize
        toolTip: qsTr("Next page")
        enabled: root.readerWorkspace.canGoForward
        onClicked: root.readerWorkspace.nextPage()
    }
}
