import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Popup {
    id: root

    required property var libraryModel
    property var sourceUrls: []
    property bool batchMode: false
    property var currentBook: ({})
    property url selectedCoverUrl: ""
    property string coverAction: "keep"
    readonly property bool batchHasChanges: titleApply.checked || authorApply.checked
                                                || seriesApply.checked || genresApply.checked
                                                || tagsApply.checked || languageApply.checked
                                                || yearApply.checked

    signal metadataUpdated

    function setBatchChecks(checked) {
        titleApply.checked = checked
        authorApply.checked = checked
        seriesApply.checked = checked
        genresApply.checked = checked
        tagsApply.checked = checked
        languageApply.checked = checked
        yearApply.checked = checked
    }

    function openFor(sourceUrl) {
        const book = root.libraryModel.book(sourceUrl)
        if (!book || !book.sourceUrl)
            return
        root.libraryModel.clearError()
        root.batchMode = false
        root.sourceUrls = [sourceUrl]
        root.currentBook = book
        root.selectedCoverUrl = ""
        root.coverAction = "keep"
        titleField.text = book.title || ""
        authorField.text = book.author || ""
        seriesField.text = book.series || ""
        seriesNumberField.text = book.seriesNumber > 0 ? String(book.seriesNumber) : ""
        genresField.text = book.genres ? book.genres.join(", ") : ""
        tagsField.text = book.tags ? book.tags.join(", ") : ""
        languageField.text = book.language || ""
        yearField.text = book.publicationYear > 0 ? String(book.publicationYear) : ""
        root.setBatchChecks(true)
        root.open()
    }

    function openForSelection() {
        const books = root.libraryModel.selectedBooks
        if (!books || books.length === 0)
            return
        root.libraryModel.clearError()
        root.batchMode = true
        root.sourceUrls = books.map(book => book.sourceUrl)
        root.currentBook = ({})
        root.selectedCoverUrl = ""
        root.coverAction = "keep"
        titleField.text = ""
        authorField.text = ""
        seriesField.text = ""
        seriesNumberField.text = ""
        genresField.text = ""
        tagsField.text = ""
        languageField.text = ""
        yearField.text = ""
        root.setBatchChecks(false)
        root.open()
    }

    function save() {
        var changes = ({})
        if (!root.batchMode || titleApply.checked)
            changes.title = titleField.text
        if (!root.batchMode || authorApply.checked)
            changes.author = authorField.text
        if (!root.batchMode || seriesApply.checked) {
            changes.series = seriesField.text
            changes.seriesNumber = seriesNumberField.text.trim().length > 0
                                   ? Number.fromLocaleString(Qt.locale(), seriesNumberField.text) : 0
        }
        if (!root.batchMode || genresApply.checked)
            changes.genres = genresField.text
        if (!root.batchMode || tagsApply.checked)
            changes.tags = tagsField.text
        if (!root.batchMode || languageApply.checked)
            changes.language = languageField.text
        if (!root.batchMode || yearApply.checked)
            changes.publicationYear = yearField.text.trim().length > 0
                                      ? Number(yearField.text) : 0

        if (Object.keys(changes).length > 0
                && !root.libraryModel.updateBooksMetadata(root.sourceUrls, changes)) {
            return
        }
        if (!root.batchMode && root.coverAction !== "keep") {
            const imageUrl = root.coverAction === "set" ? root.selectedCoverUrl : ""
            if (!root.libraryModel.setCustomCover(root.sourceUrls[0], imageUrl))
                return
        }
        root.metadataUpdated()
        root.close()
    }

    parent: Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.max(Theme.spaceMd, Math.round((parent.height - height) / 2)) : 0
    width: Math.max(0, Math.min(620, (parent ? parent.width : 652) - Theme.spaceXl))
    height: Math.max(0, Math.min(720, (parent ? parent.height : 752) - Theme.spaceXl))
    padding: Theme.spaceLg
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: Theme.darkMode ? "#99000000" : "#66000000"
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.radiusLg
        border.color: Theme.borderColor
        border.width: 1
    }

    FileDialog {
        id: coverFileDialog

        title: qsTr("Choose a cover image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.bmp)")]
        onAccepted: {
            root.selectedCoverUrl = selectedFile
            root.coverAction = "set"
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space2xs

                Label {
                    Layout.fillWidth: true
                    text: root.batchMode
                          ? qsTr("Edit %n selected book(s)", "", root.sourceUrls.length)
                          : qsTr("Book details")
                    color: Theme.textColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.titleFontSize
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.batchMode
                    text: qsTr("Choose the fields that should change for every selected book.")
                    color: Theme.mutedTextColor
                    font.family: Theme.uiFontFamily
                    font.pixelSize: Theme.captionFontSize
                    wrapMode: Text.Wrap
                }
            }

            SZHIconButton {
                symbol: "\u00d7"
                toolTip: qsTr("Close")
                onClicked: root.close()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: Theme.spaceMd

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Title"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                        Item { Layout.fillWidth: true }
                        SZHCheckBox { id: titleApply; visible: root.batchMode; text: qsTr("Apply") }
                    }
                    SZHTextField { id: titleField; objectName: "metadataTitleField"; Layout.fillWidth: true; enabled: !root.batchMode || titleApply.checked; placeholderText: qsTr("Book title") }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Author"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                        Item { Layout.fillWidth: true }
                        SZHCheckBox { id: authorApply; visible: root.batchMode; text: qsTr("Apply") }
                    }
                    SZHTextField { id: authorField; Layout.fillWidth: true; enabled: !root.batchMode || authorApply.checked; placeholderText: qsTr("Author") }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceXs
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Series"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                            Item { Layout.fillWidth: true }
                            SZHCheckBox { id: seriesApply; visible: root.batchMode; text: qsTr("Apply") }
                        }
                        SZHTextField { id: seriesField; Layout.fillWidth: true; enabled: !root.batchMode || seriesApply.checked; placeholderText: qsTr("Series name") }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 128
                        spacing: Theme.spaceXs
                        Label { text: qsTr("Number"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                        SZHTextField {
                            id: seriesNumberField
                            Layout.fillWidth: true
                            enabled: !root.batchMode || seriesApply.checked
                            placeholderText: qsTr("No.")
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            validator: DoubleValidator { bottom: 0; top: 100000; decimals: 3 }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Genres"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                        Item { Layout.fillWidth: true }
                        SZHCheckBox { id: genresApply; visible: root.batchMode; text: qsTr("Apply") }
                    }
                    SZHTextField { id: genresField; Layout.fillWidth: true; enabled: !root.batchMode || genresApply.checked; placeholderText: qsTr("Fiction, History") }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Tags"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                        Item { Layout.fillWidth: true }
                        SZHCheckBox { id: tagsApply; visible: root.batchMode; text: qsTr("Apply") }
                    }
                    SZHTextField { id: tagsField; Layout.fillWidth: true; enabled: !root.batchMode || tagsApply.checked; placeholderText: qsTr("Favorites, To review") }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceXs
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Language"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                            Item { Layout.fillWidth: true }
                            SZHCheckBox { id: languageApply; visible: root.batchMode; text: qsTr("Apply") }
                        }
                        SZHTextField { id: languageField; Layout.fillWidth: true; enabled: !root.batchMode || languageApply.checked; placeholderText: qsTr("English") }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 160
                        spacing: Theme.spaceXs
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("Year"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize }
                            Item { Layout.fillWidth: true }
                            SZHCheckBox { id: yearApply; visible: root.batchMode; text: qsTr("Apply") }
                        }
                        SZHTextField {
                            id: yearField
                            Layout.fillWidth: true
                            enabled: !root.batchMode || yearApply.checked
                            placeholderText: qsTr("Publication year")
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator { bottom: 0; top: 9999 }
                        }
                    }
                }

                Rectangle {
                    visible: !root.batchMode
                    Layout.fillWidth: true
                    implicitHeight: coverRow.implicitHeight + Theme.spaceMd * 2
                    color: Theme.surfaceMutedColor
                    radius: Theme.radiusMd

                    RowLayout {
                        id: coverRow
                        anchors.fill: parent
                        anchors.margins: Theme.spaceMd
                        spacing: Theme.spaceMd

                        LibraryBookCover {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 112
                            title: root.currentBook.title || ""
                            formatName: root.currentBook.formatName || ""
                            coverUrl: root.coverAction === "set"
                                      ? root.selectedCoverUrl
                                      : root.coverAction === "clear"
                                        ? "" : (root.currentBook.coverUrl || "")
                            fallbackColor: Theme.accentColor
                            compact: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceXs
                            Label { text: qsTr("Custom cover"); color: Theme.textColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.bodyFontSize; font.weight: Font.DemiBold }
                            Label { Layout.fillWidth: true; text: qsTr("PNG, JPEG, WebP or BMP up to 20 MB"); color: Theme.mutedTextColor; font.family: Theme.uiFontFamily; font.pixelSize: Theme.captionFontSize; wrapMode: Text.Wrap }
                            RowLayout {
                                spacing: Theme.spaceXs
                                SZHButton { text: qsTr("Choose image"); variant: "secondary"; onClicked: coverFileDialog.open() }
                                SZHButton { text: qsTr("Remove cover"); variant: "secondary"; enabled: root.coverAction === "set" || (root.currentBook.customCoverUrl || "").toString().length > 0; onClicked: { root.selectedCoverUrl = ""; root.coverAction = "clear" } }
                            }
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.libraryModel.errorMessage.length > 0
            text: root.libraryModel.errorMessage
            color: Theme.dangerColor
            font.family: Theme.uiFontFamily
            font.pixelSize: Theme.captionFontSize
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs
            Item { Layout.fillWidth: true }
            SZHButton { text: qsTr("Cancel"); variant: "secondary"; onClicked: root.close() }
            SZHButton { objectName: "saveMetadataButton"; text: qsTr("Save"); enabled: !root.batchMode || root.batchHasChanges; onClicked: root.save() }
        }
    }
}
