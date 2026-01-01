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
            Layout.maximumWidth: 300
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingMedium
                spacing: Theme.spacingMedium

                Text {
                    text: "Languages"
                    font: Theme.subtitleFont
                    color: Theme.textColor
                }

                ListView {
                    id: languageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: TranslationManager.availableLanguages

                    delegate: ItemDelegate {
                        width: languageList.width
                        height: 44
                        highlighted: modelData === TranslationManager.currentLanguage

                        Accessible.role: Accessible.Button
                        Accessible.name: {
                            var display = TranslationManager.getLanguageDisplayName(modelData)
                            var nativeName = TranslationManager.getLanguageNativeName(modelData)
                            var name = nativeName !== display ? display + ", " + nativeName : display
                            if (highlighted) name += ", selected"
                            return name
                        }
                        Accessible.description: "Select " + TranslationManager.getLanguageDisplayName(modelData) + " language"

                        background: Rectangle {
                            color: highlighted ? Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.2) : "transparent"
                            radius: Theme.buttonRadius
                        }

                        contentItem: RowLayout {
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: {
                                    var display = TranslationManager.getLanguageDisplayName(modelData)
                                    var nativeName = TranslationManager.getLanguageNativeName(modelData)
                                    return nativeName !== display ? display + " (" + nativeName + ")" : display
                                }
                                font: Theme.bodyFont
                                color: highlighted ? Theme.primaryColor : Theme.textColor
                                elide: Text.ElideRight
                            }

                            // Percentage for non-English languages
                            Text {
                                visible: modelData !== "en"
                                text: {
                                    // This is approximate - would need per-language tracking for accuracy
                                    if (modelData === TranslationManager.currentLanguage) {
                                        var total = TranslationManager.totalStringCount
                                        if (total === 0) return ""
                                        var translated = total - TranslationManager.untranslatedCount
                                        return Math.round((translated / total) * 100) + "%"
                                    }
                                    return ""
                                }
                                font: Theme.labelFont
                                color: Theme.textSecondaryColor
                            }
                        }

                        onClicked: TranslationManager.currentLanguage = modelData
                    }
                }

                // Add / Download buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Button {
                        Layout.fillWidth: true
                        text: "Add..."

                        Accessible.role: Accessible.Button
                        Accessible.name: "Add language"
                        Accessible.description: "Add a new language for translation"

                        onClicked: pageStack.push("AddLanguagePage.qml")

                        background: Rectangle {
                            implicitHeight: 40
                            color: parent.down ? Qt.darker(Theme.backgroundColor, 1.1) : Theme.backgroundColor
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
                        Layout.fillWidth: true
                        text: TranslationManager.downloading ? "..." : "Download"
                        enabled: !TranslationManager.downloading

                        Accessible.role: Accessible.Button
                        Accessible.name: TranslationManager.downloading ? "Downloading" : "Download community translations"
                        Accessible.description: "Download translations from the community"

                        background: Rectangle {
                            implicitHeight: 40
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
        }

        // Right column: Translation progress & actions
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
                    text: "Translation"
                    font: Theme.subtitleFont
                    color: Theme.textColor
                }

                // Progress card (non-English only)
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: progressColumn.height + 24
                    color: Theme.backgroundColor
                    radius: Theme.buttonRadius
                    visible: TranslationManager.currentLanguage !== "en"

                    ColumnLayout {
                        id: progressColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: TranslationManager.getLanguageDisplayName(TranslationManager.currentLanguage)
                                font.family: Theme.bodyFont.family
                                font.pixelSize: Theme.bodyFont.pixelSize
                                font.bold: true
                                color: Theme.textColor
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: {
                                    var total = TranslationManager.totalStringCount
                                    var translated = total - TranslationManager.untranslatedCount
                                    return translated + " / " + total
                                }
                                font: Theme.labelFont
                                color: Theme.textSecondaryColor
                            }
                        }

                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, TranslationManager.totalStringCount)
                            value: TranslationManager.totalStringCount - TranslationManager.untranslatedCount

                            Accessible.role: Accessible.ProgressBar
                            Accessible.name: {
                                var total = TranslationManager.totalStringCount
                                var translated = total - TranslationManager.untranslatedCount
                                var percent = Math.round((translated / Math.max(1, total)) * 100)
                                return "Translation progress: " + percent + " percent, " + translated + " of " + total + " strings"
                            }

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
                            text: {
                                var total = TranslationManager.totalStringCount
                                var translated = total - TranslationManager.untranslatedCount
                                var percent = Math.round((translated / Math.max(1, total)) * 100)
                                if (percent === 100) return "Translation complete!"
                                return TranslationManager.untranslatedCount + " strings need translation"
                            }
                            font: Theme.labelFont
                            color: Theme.textSecondaryColor
                        }
                    }
                }

                // English info
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 50
                    color: Theme.backgroundColor
                    radius: Theme.buttonRadius
                    visible: TranslationManager.currentLanguage === "en"

                    Text {
                        anchors.centerIn: parent
                        text: "English is the base language.\nYou can customize the default text below."
                        font: Theme.bodyFont
                        color: Theme.textSecondaryColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Browse strings button
                Button {
                    Layout.fillWidth: true
                    text: TranslationManager.currentLanguage === "en" ? "Browse & Customize Strings..." : "Browse & Translate Strings..."

                    Accessible.role: Accessible.Button
                    Accessible.name: TranslationManager.currentLanguage === "en" ? "Browse and customize strings" : "Browse and translate strings"
                    Accessible.description: TranslationManager.currentLanguage === "en" ? "Open the string browser to customize English text" : "Open the translation browser to translate individual strings"

                    background: Rectangle {
                        implicitHeight: 48
                        color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                        radius: Theme.buttonRadius
                    }

                    contentItem: Text {
                        text: parent.text
                        font: Theme.bodyFont
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: pageStack.push("StringBrowserPage.qml")
                }

                Item { Layout.fillHeight: true }

                // Submit to community button (not for English)
                Button {
                    Layout.fillWidth: true
                    text: "Submit to Community"
                    visible: TranslationManager.currentLanguage !== "en"

                    Accessible.role: Accessible.Button
                    Accessible.name: "Submit to community"
                    Accessible.description: "Share your translations with the community on GitHub"

                    background: Rectangle {
                        implicitHeight: 48
                        color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                        radius: Theme.buttonRadius
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
}
