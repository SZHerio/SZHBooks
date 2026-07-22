import QtQuick
import QtQuick.Layouts

GridLayout {
    id: root

    required property var libraryModel
    required property var syncService
    property bool showCollectionControl: true

    function formatOptions() {
        var options = [{"value": "all", "label": qsTr("All formats")}]
        const formats = root.libraryModel.availableFormats
        for (var index = 0; index < formats.length; ++index) {
            options.push({"value": formats[index], "label": formats[index]})
        }
        return options
    }

    function collectionOptions() {
        var options = [{"value": "", "label": qsTr("All folders")}]
        if (!root.syncService.configured) {
            return options
        }
        options.push({"value": ".", "label": qsTr("Library root")})
        const collections = root.syncService.collections
        for (var index = 0; index < collections.length; ++index) {
            options.push({
                "value": collections[index],
                "label": collections[index].split("/").join("  /  ")
            })
        }
        return options
    }

    function metadataOptions(values, allLabel) {
        var options = [{"value": "all", "label": allLabel}]
        for (var index = 0; index < values.length; ++index) {
            options.push({"value": values[index], "label": values[index]})
        }
        return options
    }

    columns: width < 720 ? 2 : width < 1160 ? 4 : root.showCollectionControl ? 8 : 7
    columnSpacing: Theme.spaceXs
    rowSpacing: Theme.spaceXs

    SZHMenuButton {
        visible: root.showCollectionControl
        Layout.fillWidth: true
        labelPrefix: qsTr("Folder")
        value: root.libraryModel.collectionFilter
        options: root.collectionOptions()
        onValueSelected: value => root.libraryModel.collectionFilter = value
    }

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Sort")
        value: root.libraryModel.sortMode
        options: [
            {"value": "recent", "label": qsTr("Recent")},
            {"value": "title", "label": qsTr("Title")},
            {"value": "author", "label": qsTr("Author")},
            {"value": "series", "label": qsTr("Series")},
            {"value": "year", "label": qsTr("Year")}
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

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Genre")
        value: root.libraryModel.genreFilter
        options: root.metadataOptions(root.libraryModel.availableGenres, qsTr("All genres"))
        onValueSelected: value => root.libraryModel.genreFilter = value
    }

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Tag")
        value: root.libraryModel.tagFilter
        options: root.metadataOptions(root.libraryModel.availableTags, qsTr("All tags"))
        onValueSelected: value => root.libraryModel.tagFilter = value
    }

    SZHMenuButton {
        Layout.fillWidth: true
        labelPrefix: qsTr("Language")
        value: root.libraryModel.languageFilter
        options: root.metadataOptions(root.libraryModel.availableLanguages, qsTr("All languages"))
        onValueSelected: value => root.libraryModel.languageFilter = value
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
