import QtQuick
import QtQuick.Layouts
import DE1App

Rectangle {
    color: Theme.surfaceColor

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.standardMargin
        anchors.rightMargin: Theme.standardMargin
        spacing: Theme.scaled(10)

        // Page title (from root.currentPageTitle)
        Text {
            text: root.currentPageTitle
            color: Theme.textColor
            font.pixelSize: Theme.scaled(16)
            font.bold: true
            Layout.preferredWidth: implicitWidth
            elide: Text.ElideRight
        }

        // Separator after title (if title exists)
        Rectangle {
            width: Theme.scaled(1)
            height: parent.height * 0.5
            color: Theme.textSecondaryColor
            opacity: 0.3
            visible: root.currentPageTitle !== ""
        }

        // Machine state
        Text {
            text: DE1Device.stateString
            color: Theme.textColor
            font.pixelSize: Theme.scaled(14)
        }

        Text {
            text: " - " + DE1Device.subStateString
            color: Theme.textSecondaryColor
            font.pixelSize: Theme.scaled(14)
            visible: MachineState.isFlowing
        }

        Item { Layout.fillWidth: true }

        // Temperature
        Text {
            text: DE1Device.temperature.toFixed(1) + "°C"
            color: Theme.temperatureColor
            font.pixelSize: Theme.scaled(14)
        }

        // Separator
        Rectangle {
            width: Theme.scaled(1)
            height: parent.height * 0.5
            color: Theme.textSecondaryColor
            opacity: 0.3
        }

        // Water level
        Text {
            text: DE1Device.waterLevel.toFixed(0) + "%"
            color: DE1Device.waterLevel > 20 ? Theme.primaryColor : Theme.warningColor
            font.pixelSize: Theme.scaled(14)
        }

        // Separator
        Rectangle {
            width: Theme.scaled(1)
            height: parent.height * 0.5
            color: Theme.textSecondaryColor
            opacity: 0.3
        }

        // Scale warning (clickable to scan)
        Rectangle {
            visible: BLEManager.scaleConnectionFailed || (BLEManager.hasSavedScale && (!ScaleDevice || !ScaleDevice.connected))
            color: BLEManager.scaleConnectionFailed ? Theme.errorColor : "transparent"
            radius: Theme.scaled(4)
            Layout.preferredHeight: parent.height * 0.7
            Layout.preferredWidth: scaleWarningRow.implicitWidth + Theme.scaled(16)

            Row {
                id: scaleWarningRow
                anchors.centerIn: parent
                spacing: Theme.scaled(5)

                Text {
                    text: BLEManager.scaleConnectionFailed ? "Scale not found" :
                          (ScaleDevice && ScaleDevice.connected ? "" : "Scale...")
                    color: BLEManager.scaleConnectionFailed ? "white" : Theme.textSecondaryColor
                    font.pixelSize: Theme.scaled(12)
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "[Scan]"
                    color: Theme.accentColor
                    font.pixelSize: Theme.scaled(12)
                    font.underline: true
                    visible: BLEManager.scaleConnectionFailed
                    anchors.verticalCenter: parent.verticalCenter

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: BLEManager.scanForScales()
                    }
                }
            }
        }

        // Scale connected indicator
        Row {
            spacing: Theme.scaled(5)
            visible: ScaleDevice && ScaleDevice.connected

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.scaled(8)
                height: Theme.scaled(8)
                radius: Theme.scaled(4)
                color: Theme.weightColor
            }

            Text {
                text: MachineState.scaleWeight.toFixed(1) + "g"
                color: Theme.weightColor
                font.pixelSize: Theme.scaled(14)
            }
        }

        // Build number
        Text {
            text: "#" + BuildNumber
            color: Theme.textSecondaryColor
            font.pixelSize: Theme.scaled(18)
            font.bold: true
            opacity: 0.6
        }

        // Separator before DE1 status
        Rectangle {
            width: Theme.scaled(1)
            height: parent.height * 0.5
            color: Theme.textSecondaryColor
            opacity: 0.3
        }

        // Connection indicator
        Row {
            spacing: Theme.scaled(5)

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.scaled(10)
                height: Theme.scaled(10)
                radius: Theme.scaled(5)
                color: DE1Device.connected ? Theme.successColor : Theme.errorColor
            }

            Text {
                text: DE1Device.connected ? "Online" : "Offline"
                color: DE1Device.connected ? Theme.successColor : Theme.textSecondaryColor
                font.pixelSize: Theme.scaled(14)
            }
        }
    }
}
