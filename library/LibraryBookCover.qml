import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    required property string title
    required property string formatName
    required property url coverUrl

    property color fallbackColor: Theme.accentColor
    property bool compact: false

    color: coverImage.status === Image.Ready ? Theme.surfaceMutedColor : fallbackColor
    radius: Theme.radiusSm
    clip: true

    Image {
        id: coverImage

        anchors.fill: parent
        source: root.coverUrl
        sourceSize.width: 720
        sourceSize.height: 1040
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
        mipmap: true
    }

    Rectangle {
        visible: coverImage.status !== Image.Ready
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.compact ? 4 : 7
        color: Qt.darker(root.fallbackColor, 1.35)
    }

    Text {
        visible: coverImage.status !== Image.Ready && !root.compact
        anchors.centerIn: parent
        width: Math.max(0, parent.width - Theme.space2xl)
        text: root.title
        color: "#ffffff"
        font.family: Theme.readingFontFamily
        font.pixelSize: Theme.bodyLargeFontSize
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        maximumLineCount: 4
        elide: Text.ElideRight
    }

    Text {
        visible: coverImage.status !== Image.Ready && root.compact
        anchors.centerIn: parent
        text: root.title.length > 0
              ? root.title.slice(0, 1).toUpperCase()
              : root.formatName.slice(0, 1).toUpperCase()
        color: "#ffffff"
        font.family: Theme.readingFontFamily
        font.pixelSize: Theme.titleFontSize
        font.weight: Font.DemiBold
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: root.compact ? Theme.space2xs : Theme.spaceXs
        width: formatLabel.implicitWidth + Theme.spaceXs
        height: formatLabel.implicitHeight + Theme.space2xs
        color: coverImage.status === Image.Ready ? "#d9111111" : "#33000000"
        radius: Theme.radiusSm

        Label {
            id: formatLabel

            anchors.centerIn: parent
            text: root.formatName
            color: "#ffffff"
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            font.weight: Font.DemiBold
        }
    }
}
