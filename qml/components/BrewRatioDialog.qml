import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DecenzaDE1

Dialog {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Theme.scaled(400)
    modal: true
    padding: 0

    // Captured dose weight when dialog opens
    property double capturedDose: 0
    property double ratio: Settings.lastUsedRatio
    property bool lowDoseWarning: capturedDose < 3

    // Calculated target weight
    readonly property double calculatedTarget: capturedDose * ratio

    onAboutToShow: {
        // Capture current scale weight when dialog opens
        capturedDose = MachineState.scaleWeight
        ratio = Settings.lastUsedRatio
    }

    background: Rectangle {
        color: Theme.surfaceColor
        radius: Theme.cardRadius
        border.width: 1
        border.color: "white"
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Header
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.scaled(50)
            Layout.topMargin: Theme.scaled(10)

            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.scaled(20)
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Brew by Ratio")
                font: Theme.titleFont
                color: Theme.textColor
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.borderColor
            }
        }

        // Content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.scaled(20)
            spacing: Theme.scaled(16)

            // Low dose warning
            Rectangle {
                Layout.fillWidth: true
                visible: root.lowDoseWarning
                color: Theme.warningColor
                radius: Theme.scaled(8)
                height: warningText.implicitHeight + Theme.scaled(16)

                Text {
                    id: warningText
                    anchors.fill: parent
                    anchors.margins: Theme.scaled(8)
                    text: qsTr("Please put the portafilter with coffee on the scale")
                    font: Theme.bodyFont
                    color: "white"
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Dose display
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.scaled(8)

                Text {
                    text: qsTr("Dose:")
                    font: Theme.bodyFont
                    color: Theme.textSecondaryColor
                }

                Text {
                    text: root.capturedDose.toFixed(1) + "g"
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.bodyFont.pixelSize
                    font.bold: true
                    color: Theme.weightColor
                }
            }

            // Ratio input
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.scaled(8)

                Text {
                    text: qsTr("Ratio: 1:")
                    font: Theme.bodyFont
                    color: Theme.textSecondaryColor
                }

                ValueInput {
                    id: ratioInput
                    Layout.fillWidth: true
                    value: root.ratio
                    from: 0.5
                    to: 4.0
                    stepSize: 0.1
                    decimals: 1
                    valueColor: Theme.primaryColor
                    accentColor: Theme.primaryColor
                    onValueModified: function(newValue) {
                        root.ratio = newValue
                    }
                }
            }

            // Target display
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.scaled(8)

                Text {
                    text: qsTr("Target:")
                    font: Theme.bodyFont
                    color: Theme.textSecondaryColor
                }

                Text {
                    text: root.calculatedTarget.toFixed(0) + "g"
                    font.family: Theme.bodyFont.family
                    font.pixelSize: Theme.bodyFont.pixelSize
                    font.bold: true
                    color: Theme.weightColor
                }
            }
        }

        // Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.scaled(20)
            Layout.rightMargin: Theme.scaled(20)
            Layout.bottomMargin: Theme.scaled(20)
            spacing: Theme.scaled(10)

            AccessibleButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaled(50)
                text: qsTr("Cancel")
                accessibleName: qsTr("Cancel brew by ratio")
                onClicked: root.reject()
                background: Rectangle {
                    implicitHeight: Theme.scaled(50)
                    radius: Theme.buttonRadius
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.textSecondaryColor
                }
                contentItem: Text {
                    text: parent.text
                    font: Theme.bodyFont
                    color: Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            AccessibleButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaled(50)
                text: qsTr("OK")
                accessibleName: qsTr("Confirm brew by ratio")
                onClicked: {
                    // Re-read weight on OK (in case user added more coffee)
                    var finalDose = MachineState.scaleWeight
                    Settings.lastUsedRatio = root.ratio
                    MainController.activateBrewByRatio(finalDose, root.ratio)
                    root.accept()
                }
                background: Rectangle {
                    implicitHeight: Theme.scaled(50)
                    radius: Theme.buttonRadius
                    color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                }
                contentItem: Text {
                    text: parent.text
                    font: Theme.bodyFont
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
