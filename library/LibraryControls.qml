import QtQuick
import QtQuick.Layouts

GridLayout {
    id: root

    required property var libraryModel

    function formatOptions() {
        var options = [{"value": "all", "label": qsTr("All formats")}]
        const formats = root.libraryModel.availableFormats
        for (var index = 0; index < formats.length; ++index) {
            options.push({"value": formats[index], "label": formats[index]})
        }
        return options
    }

    columns: width < 820 ? 2 : 4
    columnSpacing: Theme.spaceXs
    rowSpacing: Theme.spaceXs

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Sort")
        value: root.libraryModel.sortMode
        options: [
            {"value": "recent", "label": qsTr("Recent")},
            {"value": "title", "label": qsTr("Title")},
            {"value": "author", "label": qsTr("Author")}
        ]
        onValueSelected: value => root.libraryModel.sortMode = value
    }

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Format")
        value: root.libraryModel.formatFilter
        options: root.formatOptions()
        onValueSelected: value => root.libraryModel.formatFilter = value
    }

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Status")
        value: root.libraryModel.progressFilter
        options: [
            {"value": "all", "label": qsTr("All")},
            {"value": "unread", "label": qsTr("Unread")},
            {"value": "reading", "label": qsTr("Reading")},
            {"value": "finished", "label": qsTr("Finished")}
        ]
        onValueSelected: value => root.libraryModel.progressFilter = value
    }

    SZHSegmentedControl {
        Layout.fillWidth: true
        model: [
            {"value": "grid", "label": qsTr("Grid")},
            {"value": "list", "label": qsTr("List")}
        ]
        value: root.libraryModel.viewMode
        onValueSelected: value => root.libraryModel.viewMode = value
    }
}
