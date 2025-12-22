import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DE1App
import "../components"

Page {
    objectName: "settingsPage"
    background: Rectangle { color: Theme.backgroundColor }

    header: ToolBar {
        background: Rectangle { color: Theme.surfaceColor }

        RowLayout {
            anchors.fill: parent

            ToolButton {
                icon.source: "qrc:/icons/back.svg"
                onClicked: root.goBack()
            }

            Label {
                text: "Settings"
                color: Theme.textColor
                font: Theme.headingFont
                Layout.fillWidth: true
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: Theme.standardMargin

        ColumnLayout {
            width: parent.width
            spacing: 20

            // Machine Connection
            GroupBox {
                Layout.fillWidth: true
                title: "Machine Connection"
                label: Text {
                    text: parent.title
                    color: Theme.textColor
                    font: Theme.bodyFont
                }

                background: Rectangle {
                    color: Theme.surfaceColor
                    radius: Theme.cardRadius
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "Status:"
                            color: Theme.textSecondaryColor
                        }

                        Text {
                            text: DE1Device.connected ? "Connected" : "Disconnected"
                            color: DE1Device.connected ? Theme.successColor : Theme.errorColor
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: BLEManager.scanning ? "Stop Scan" : "Scan"
                            onClicked: {
                                if (BLEManager.scanning) {
                                    BLEManager.stopScan()
                                } else {
                                    BLEManager.startScan()
                                }
                            }
                        }
                    }

                    Text {
                        text: "Firmware: " + (DE1Device.firmwareVersion || "Unknown")
                        color: Theme.textSecondaryColor
                        visible: DE1Device.connected
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                        clip: true
                        model: BLEManager.discoveredDevices

                        delegate: ItemDelegate {
                            width: ListView.view.width
                            contentItem: Text {
                                text: modelData.name + " (" + modelData.address + ")"
                                color: Theme.textColor
                            }
                            background: Rectangle {
                                color: parent.hovered ? Theme.accentColor : "transparent"
                                radius: 4
                            }
                            onClicked: DE1Device.connectToDevice(modelData.address)
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "No devices found"
                            visible: parent.count === 0
                            color: Theme.textSecondaryColor
                        }
                    }
                }
            }

            // Scale Connection
            GroupBox {
                Layout.fillWidth: true
                title: "Scale Connection"
                label: Text {
                    text: parent.title
                    color: Theme.textColor
                    font: Theme.bodyFont
                }

                background: Rectangle {
                    color: Theme.surfaceColor
                    radius: Theme.cardRadius
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "Status:"
                            color: Theme.textSecondaryColor
                        }

                        Text {
                            text: ScaleDevice.connected ? "Connected" : "Disconnected"
                            color: ScaleDevice.connected ? Theme.successColor : Theme.textSecondaryColor
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: BLEManager.scanning ? "Scanning..." : "Scan"
                            enabled: !BLEManager.scanning
                            onClicked: BLEManager.startScan()
                        }
                    }

                    // Show weight when connected
                    RowLayout {
                        Layout.fillWidth: true
                        visible: ScaleDevice.connected

                        Text {
                            text: "Weight:"
                            color: Theme.textSecondaryColor
                        }

                        Text {
                            text: ScaleDevice.weight.toFixed(1) + " g"
                            color: Theme.textColor
                            font: Theme.bodyFont
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Tare"
                            onClicked: ScaleDevice.tare()
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(150, Math.max(50, count * 50))
                        clip: true
                        visible: !ScaleDevice.connected
                        model: BLEManager.discoveredScales

                        delegate: ItemDelegate {
                            width: ListView.view.width
                            contentItem: RowLayout {
                                Text {
                                    text: modelData.name
                                    color: Theme.textColor
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.type
                                    color: Theme.textSecondaryColor
                                    font.pixelSize: 12
                                }
                            }
                            background: Rectangle {
                                color: parent.hovered ? Theme.accentColor : "transparent"
                                radius: 4
                            }
                            onClicked: {
                                console.log("Connect to scale:", modelData.name, modelData.type)
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "No scales found"
                            visible: parent.count === 0
                            color: Theme.textSecondaryColor
                        }
                    }
                }
            }

            // About
            GroupBox {
                Layout.fillWidth: true
                title: "About"
                label: Text {
                    text: parent.title
                    color: Theme.textColor
                    font: Theme.bodyFont
                }

                background: Rectangle {
                    color: Theme.surfaceColor
                    radius: Theme.cardRadius
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 5

                    Text {
                        text: "DE1 Controller"
                        color: Theme.textColor
                        font: Theme.bodyFont
                    }

                    Text {
                        text: "Version 1.0.0"
                        color: Theme.textSecondaryColor
                    }

                    Text {
                        text: "Built with Qt 6"
                        color: Theme.textSecondaryColor
                    }
                }
            }

            Item { Layout.preferredHeight: 50 }
        }
    }
}
