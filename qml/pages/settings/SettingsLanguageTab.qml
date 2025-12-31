import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1
import "../../components"

Item {
    id: languageTab

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingMedium

        // Left column: Language selection
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: Theme.settingsColumnMax
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingMedium
                spacing: Theme.spacingMedium

                // Header
                Text {
                    text: "Languages"
                    font: Theme.titleFont
                    color: Theme.textColor
                }

                // Language list
                ListView {
                    id: languageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: TranslationManager.availableLanguages

                    delegate: ItemDelegate {
                        width: languageList.width
                        height: Theme.touchTargetMin
                        highlighted: modelData === TranslationManager.currentLanguage

                        background: Rectangle {
                            color: highlighted ? Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.2) : "transparent"
                            radius: 4
                        }

                        contentItem: RowLayout {
                            spacing: Theme.spacingSmall

                            Text {
                                Layout.fillWidth: true
                                text: TranslationManager.getLanguageDisplayName(modelData) +
                                      (TranslationManager.getLanguageNativeName(modelData) !== TranslationManager.getLanguageDisplayName(modelData) ?
                                       " (" + TranslationManager.getLanguageNativeName(modelData) + ")" : "")
                                font: Theme.bodyFont
                                color: highlighted ? Theme.primaryColor : Theme.textColor
                                elide: Text.ElideRight
                            }

                            // Progress indicator for non-English
                            Text {
                                visible: modelData !== "en" && modelData === TranslationManager.currentLanguage
                                text: {
                                    var total = TranslationManager.totalStringCount
                                    if (total === 0) return ""
                                    var translated = total - TranslationManager.untranslatedCount
                                    return Math.round((translated / total) * 100) + "%"
                                }
                                font: Theme.labelFont
                                color: Theme.textSecondaryColor
                            }
                        }

                        onClicked: TranslationManager.currentLanguage = modelData
                    }
                }

                // Add language button
                Button {
                    Layout.fillWidth: true
                    text: "Add Language..."
                    onClicked: addLanguageDialog.open()

                    background: Rectangle {
                        implicitHeight: Theme.touchTargetMin
                        color: parent.down ? Qt.darker(Theme.surfaceColor, 1.2) : Theme.backgroundColor
                        radius: Theme.buttonRadius
                        border.width: 1
                        border.color: Theme.borderColor
                    }

                    contentItem: Text {
                        text: parent.text
                        font: Theme.bodyFont
                        color: Theme.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Download languages button
                Button {
                    Layout.fillWidth: true
                    text: TranslationManager.downloading ? "Downloading..." : "Download Community Languages"
                    enabled: !TranslationManager.downloading

                    background: Rectangle {
                        implicitHeight: Theme.touchTargetMin
                        color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                        radius: Theme.buttonRadius
                        opacity: parent.enabled ? 1.0 : 0.5
                    }

                    contentItem: Text {
                        text: parent.text
                        font: Theme.bodyFont
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: TranslationManager.downloadLanguageList()
                }
            }
        }

        // Right column: Translation tools
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingMedium
                spacing: Theme.spacingMedium

                Text {
                    text: "Translation Tools"
                    font: Theme.titleFont
                    color: Theme.textColor
                }

                // Edit mode toggle
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: editModeRow.implicitHeight + Theme.spacingMedium * 2
                    color: Theme.backgroundColor
                    radius: 4

                    RowLayout {
                        id: editModeRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.spacingMedium

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Edit Mode"
                                font: Theme.subtitleFont
                                color: Theme.textColor
                            }

                            Text {
                                text: "Click any text in the app to translate it"
                                font: Theme.labelFont
                                color: Theme.textSecondaryColor
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        Switch {
                            checked: TranslationManager.editModeEnabled
                            onCheckedChanged: TranslationManager.editModeEnabled = checked
                        }
                    }
                }

                // Translation status
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: translationStatusColumn.height + Theme.spacingMedium * 2
                    color: Theme.backgroundColor
                    radius: 4
                    visible: TranslationManager.currentLanguage !== "en"

                    ColumnLayout {
                        id: translationStatusColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.spacingMedium
                        spacing: 8

                        Text {
                            text: "Translation Status: " + TranslationManager.getLanguageDisplayName(TranslationManager.currentLanguage)
                            font: Theme.subtitleFont
                            color: Theme.textColor
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, TranslationManager.totalStringCount)
                            value: TranslationManager.totalStringCount - TranslationManager.untranslatedCount

                            background: Rectangle {
                                implicitHeight: 8
                                color: Theme.borderColor
                                radius: 4
                            }

                            contentItem: Item {
                                Rectangle {
                                    width: parent.parent.visualPosition * parent.width
                                    height: parent.height
                                    radius: 4
                                    color: Theme.successColor
                                }
                            }
                        }

                        Text {
                            text: (TranslationManager.totalStringCount - TranslationManager.untranslatedCount) +
                                  " of " + TranslationManager.totalStringCount + " strings translated"
                            font: Theme.labelFont
                            color: Theme.textSecondaryColor
                        }
                    }
                }

                // Current language info
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: langInfoColumn.height + Theme.spacingMedium * 2
                    color: Theme.backgroundColor
                    radius: 4
                    visible: TranslationManager.currentLanguage === "en"

                    ColumnLayout {
                        id: langInfoColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.spacingMedium
                        spacing: 4

                        Text {
                            text: "English (Source Language)"
                            font: Theme.subtitleFont
                            color: Theme.textColor
                        }

                        Text {
                            text: "Select a different language to start translating, or add a new language."
                            font: Theme.labelFont
                            color: Theme.textSecondaryColor
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Export/Import buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMedium

                    Button {
                        Layout.fillWidth: true
                        text: "Export"
                        enabled: TranslationManager.currentLanguage !== "en"

                        background: Rectangle {
                            implicitHeight: Theme.touchTargetMin
                            color: parent.down ? Qt.darker(Theme.surfaceColor, 1.2) : Theme.backgroundColor
                            radius: Theme.buttonRadius
                            border.width: 1
                            border.color: Theme.borderColor
                            opacity: parent.enabled ? 1.0 : 0.5
                        }

                        contentItem: Text {
                            text: parent.text
                            font: Theme.bodyFont
                            color: Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            opacity: parent.enabled ? 1.0 : 0.5
                        }

                        onClicked: {
                            // Export to downloads folder
                            var filename = TranslationManager.currentLanguage + "_translation.json"
                            TranslationManager.exportTranslation(filename)
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Import"

                        background: Rectangle {
                            implicitHeight: Theme.touchTargetMin
                            color: parent.down ? Qt.darker(Theme.surfaceColor, 1.2) : Theme.backgroundColor
                            radius: Theme.buttonRadius
                            border.width: 1
                            border.color: Theme.borderColor
                        }

                        contentItem: Text {
                            text: parent.text
                            font: Theme.bodyFont
                            color: Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            // TODO: Open file picker
                            console.log("Import not yet implemented - use file manager to copy JSON to app data folder")
                        }
                    }
                }

                // Submit to community button
                Button {
                    Layout.fillWidth: true
                    text: "Submit to Community"
                    enabled: TranslationManager.currentLanguage !== "en"

                    background: Rectangle {
                        implicitHeight: Theme.touchTargetMin
                        color: parent.down ? Qt.darker(Theme.successColor, 1.2) : Theme.successColor
                        radius: Theme.buttonRadius
                        opacity: parent.enabled ? 1.0 : 0.5
                    }

                    contentItem: Text {
                        text: parent.text
                        font: Theme.bodyFont
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: TranslationManager.openGitHubSubmission()
                }
            }
        }
    }

    // Add Language Dialog
    Dialog {
        id: addLanguageDialog
        title: "Add New Language"
        modal: true
        anchors.centerIn: parent
        width: Math.min(350, parent.width - 40)

        background: Rectangle {
            color: Theme.surfaceColor
            radius: Theme.cardRadius
            border.width: 1
            border.color: Theme.borderColor
        }

        header: Rectangle {
            color: "transparent"
            height: 50
            Text {
                anchors.centerIn: parent
                text: addLanguageDialog.title
                font: Theme.titleFont
                color: Theme.textColor
            }
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingMedium

            Text {
                text: "Language Code (e.g., de, fr, es):"
                font: Theme.labelFont
                color: Theme.textSecondaryColor
            }

            StyledTextField {
                id: langCodeInput
                Layout.fillWidth: true
                placeholderText: "en"
                maximumLength: 5
            }

            Text {
                text: "Display Name (e.g., German, French):"
                font: Theme.labelFont
                color: Theme.textSecondaryColor
            }

            StyledTextField {
                id: langNameInput
                Layout.fillWidth: true
                placeholderText: "English"
            }

            Text {
                text: "Native Name (e.g., Deutsch, Francais):"
                font: Theme.labelFont
                color: Theme.textSecondaryColor
            }

            StyledTextField {
                id: langNativeNameInput
                Layout.fillWidth: true
                placeholderText: "Optional"
            }
        }

        footer: RowLayout {
            spacing: Theme.spacingMedium

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: addLanguageDialog.close()

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: Theme.touchTargetMin
                    color: parent.down ? Qt.darker(Theme.surfaceColor, 1.2) : Theme.surfaceColor
                    radius: Theme.buttonRadius
                    border.width: 1
                    border.color: Theme.borderColor
                }

                contentItem: Text {
                    text: parent.text
                    font: Theme.bodyFont
                    color: Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                text: "Add"
                enabled: langCodeInput.text.trim().length >= 2 && langNameInput.text.trim().length >= 2

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: Theme.touchTargetMin
                    color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                    radius: Theme.buttonRadius
                    opacity: parent.enabled ? 1.0 : 0.5
                }

                contentItem: Text {
                    text: parent.text
                    font: Theme.bodyFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    TranslationManager.addLanguage(
                        langCodeInput.text.trim().toLowerCase(),
                        langNameInput.text.trim(),
                        langNativeNameInput.text.trim()
                    )
                    langCodeInput.text = ""
                    langNameInput.text = ""
                    langNativeNameInput.text = ""
                    addLanguageDialog.close()
                }
            }
        }
    }
}
