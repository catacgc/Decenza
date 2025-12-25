import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DE1App

Item {
    id: root

    // Value properties
    property real value: 0
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: stepSize < 1 ? 1 : 0

    // Display
    property string suffix: ""
    property color valueColor: Theme.textColor
    property color accentColor: Theme.primaryColor

    // Signals - emits the new value for parent to apply
    signal valueModified(real newValue)

    implicitWidth: Theme.scaled(120)
    implicitHeight: Theme.scaled(56)

    // Compact value display
    Rectangle {
        id: valueDisplay
        anchors.fill: parent
        radius: Theme.scaled(12)
        color: Theme.surfaceColor
        border.width: 1
        border.color: Theme.borderColor

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.scaled(4)
            spacing: Theme.scaled(2)

            // Minus button
            Rectangle {
                Layout.preferredWidth: Theme.scaled(40)
                Layout.fillHeight: true
                radius: Theme.scaled(8)
                color: minusArea.pressed ? Qt.darker(Theme.surfaceColor, 1.3) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "\u2212"
                    font.pixelSize: Theme.scaled(22)
                    font.bold: true
                    color: root.value <= root.from ? Theme.textSecondaryColor : Theme.textColor
                }

                MouseArea {
                    id: minusArea
                    anchors.fill: parent
                    onClicked: adjustValue(-1)
                    onPressAndHold: decrementTimer.start()
                    onReleased: decrementTimer.stop()
                    onCanceled: decrementTimer.stop()
                }

                Timer {
                    id: decrementTimer
                    interval: 80
                    repeat: true
                    onTriggered: adjustValue(-1)
                }
            }

            // Value display - drag to adjust, tap to open scrubber
            Item {
                id: valueContainer
                Layout.fillWidth: true
                Layout.fillHeight: true

                Text {
                    id: valueText
                    anchors.centerIn: parent
                    text: root.value.toFixed(root.decimals) + root.suffix
                    font: Theme.headingFont
                    color: valueDragArea.isDragging ? root.accentColor : root.valueColor
                    opacity: valueDragArea.isDragging ? 0.3 : 1.0
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                }

                MouseArea {
                    id: valueDragArea
                    anchors.fill: parent

                    property real startX: 0
                    property real startY: 0
                    property real currentX: 0
                    property real accumulator: 0
                    property bool isDragging: false
                    property real pixelsPerStep: 20

                    drag.target: Item {}  // Enable drag detection
                    drag.axis: Drag.XAndYAxis
                    drag.threshold: 5

                    onPressed: function(mouse) {
                        startX = mouse.x
                        startY = mouse.y
                        currentX = mouse.x
                        accumulator = 0
                        isDragging = false
                    }

                    onPositionChanged: function(mouse) {
                        currentX = mouse.x
                        var deltaX = mouse.x - startX
                        var deltaY = startY - mouse.y  // Inverted: up = increase

                        // Use whichever axis has more movement
                        var delta = Math.abs(deltaX) > Math.abs(deltaY) ? deltaX : deltaY

                        if (Math.abs(delta) > drag.threshold) {
                            isDragging = true
                        }

                        if (isDragging) {
                            accumulator += delta
                            var steps = Math.floor(accumulator / pixelsPerStep)
                            if (steps !== 0) {
                                adjustValue(steps)
                                accumulator -= steps * pixelsPerStep
                            }
                            startX = mouse.x
                            startY = mouse.y
                        }
                    }

                    onReleased: {
                        if (!isDragging) {
                            scrubberPopup.open()
                        }
                        isDragging = false
                        accumulator = 0
                    }

                    onCanceled: {
                        isDragging = false
                        accumulator = 0
                    }
                }

                // Anchor point for bubble positioning (centered)
                Item {
                    id: bubbleAnchor
                    anchors.centerIn: parent
                    width: 1
                    height: 1
                }

                // Floating speech bubble - rendered in overlay to be always on top
                Loader {
                    id: bubbleLoader
                    active: valueDragArea.isDragging
                    sourceComponent: Item {
                        id: speechBubble
                        parent: Overlay.overlay
                        visible: valueDragArea.isDragging

                        // Calculate luminance to determine text color
                        function getContrastColor(c) {
                            var luminance = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b
                            return luminance > 0.5 ? "#000000" : "#FFFFFF"
                        }

                        property point globalPos: bubbleAnchor.mapToGlobal(0, 0)
                        x: globalPos.x - width / 2
                        y: globalPos.y - height - Theme.scaled(15)
                        width: bubbleRect.width
                        height: bubbleRect.height + bubbleTail.height - Theme.scaled(3)

                        // Pop-in animation
                        scale: valueDragArea.isDragging ? 1.0 : 0.5
                        opacity: valueDragArea.isDragging ? 1.0 : 0
                        transformOrigin: Item.Bottom
                        Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutBack; easing.overshoot: 2 } }
                        Behavior on opacity { NumberAnimation { duration: 100 } }

                        // Bubble body - 1.5x larger
                        Rectangle {
                            id: bubbleRect
                            width: bubbleText.width + Theme.scaled(36)
                            height: Theme.scaled(66)
                            radius: height / 2
                            color: root.valueColor

                            // Subtle gradient shine
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: Theme.scaled(3)
                                radius: parent.radius - Theme.scaled(3)
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.3) }
                                    GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0) }
                                }
                            }

                            Text {
                                id: bubbleText
                                anchors.centerIn: parent
                                text: root.value.toFixed(root.decimals) + root.suffix
                                font.pixelSize: Theme.scaled(30)
                                font.bold: true
                                color: speechBubble.getContrastColor(root.valueColor)
                            }
                        }

                        // Cartoon tail (triangle pointing down)
                        Canvas {
                            id: bubbleTail
                            anchors.horizontalCenter: bubbleRect.horizontalCenter
                            anchors.top: bubbleRect.bottom
                            anchors.topMargin: -Theme.scaled(3)
                            width: Theme.scaled(30)
                            height: Theme.scaled(21)

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.fillStyle = root.valueColor
                                ctx.beginPath()
                                ctx.moveTo(0, 0)
                                ctx.lineTo(width, 0)
                                ctx.lineTo(width / 2, height)
                                ctx.closePath()
                                ctx.fill()
                            }

                            // Redraw when color changes
                            Connections {
                                target: root
                                function onValueColorChanged() { bubbleTail.requestPaint() }
                            }
                            Component.onCompleted: requestPaint()
                        }
                    }
                }
            }

            // Plus button
            Rectangle {
                Layout.preferredWidth: Theme.scaled(40)
                Layout.fillHeight: true
                radius: Theme.scaled(8)
                color: plusArea.pressed ? Qt.darker(Theme.surfaceColor, 1.3) : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "+"
                    font.pixelSize: Theme.scaled(22)
                    font.bold: true
                    color: root.value >= root.to ? Theme.textSecondaryColor : Theme.textColor
                }

                MouseArea {
                    id: plusArea
                    anchors.fill: parent
                    onClicked: adjustValue(1)
                    onPressAndHold: incrementTimer.start()
                    onReleased: incrementTimer.stop()
                    onCanceled: incrementTimer.stop()
                }

                Timer {
                    id: incrementTimer
                    interval: 80
                    repeat: true
                    onTriggered: adjustValue(1)
                }
            }
        }
    }

    // Full-screen scrubber popup
    Popup {
        id: scrubberPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        modal: true
        dim: true
        closePolicy: Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#CC000000"
        }

        // Scrubber content
        Item {
            anchors.fill: parent

            // Header with current value
            Rectangle {
                id: header
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: Theme.scaled(60)
                width: Theme.scaled(200)
                height: Theme.scaled(80)
                radius: Theme.scaled(16)
                color: root.accentColor

                Text {
                    anchors.centerIn: parent
                    text: root.value.toFixed(root.decimals) + root.suffix
                    font.pixelSize: Theme.scaled(36)
                    font.bold: true
                    color: "white"
                }
            }

            // Range display
            Text {
                anchors.top: header.bottom
                anchors.topMargin: Theme.scaled(12)
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.from.toFixed(root.decimals) + " \u2014 " + root.to.toFixed(root.decimals)
                font: Theme.captionFont
                color: "#888888"
            }

            // Vertical scrubber track
            Item {
                id: scrubberTrack
                anchors.centerIn: parent
                width: Theme.scaled(280)
                height: Theme.scaled(400)

                // Background track
                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.scaled(80)
                    height: parent.height
                    radius: Theme.scaled(40)
                    color: "#333333"

                    // Velocity zone indicators
                    Column {
                        anchors.fill: parent
                        anchors.margins: Theme.scaled(4)

                        // Fast up zone
                        Rectangle {
                            width: parent.width
                            height: parent.height * 0.2
                            radius: Theme.scaled(36)
                            color: scrubberArea.pressed && scrubberArea.velocityZone === 2 ? "#664CAF50" : "#22FFFFFF"
                            Text {
                                anchors.centerIn: parent
                                text: "\u25B2\u25B2"
                                color: "#666666"
                                font.pixelSize: Theme.scaled(16)
                            }
                        }

                        // Medium up zone
                        Rectangle {
                            width: parent.width
                            height: parent.height * 0.15
                            color: scrubberArea.pressed && scrubberArea.velocityZone === 1 ? "#442196F3" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u25B2"
                                color: "#666666"
                                font.pixelSize: Theme.scaled(14)
                            }
                        }

                        // Fine zone (center)
                        Rectangle {
                            width: parent.width
                            height: parent.height * 0.3
                            color: scrubberArea.pressed && scrubberArea.velocityZone === 0 ? "#44FFFFFF" : "transparent"

                            // Center handle
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width - Theme.scaled(16)
                                height: Theme.scaled(60)
                                radius: Theme.scaled(30)
                                color: root.accentColor

                                Text {
                                    anchors.centerIn: parent
                                    text: "DRAG"
                                    font.pixelSize: Theme.scaled(14)
                                    font.bold: true
                                    color: "white"
                                    opacity: scrubberArea.pressed ? 0 : 0.7
                                }
                            }
                        }

                        // Medium down zone
                        Rectangle {
                            width: parent.width
                            height: parent.height * 0.15
                            color: scrubberArea.pressed && scrubberArea.velocityZone === -1 ? "#442196F3" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u25BC"
                                color: "#666666"
                                font.pixelSize: Theme.scaled(14)
                            }
                        }

                        // Fast down zone
                        Rectangle {
                            width: parent.width
                            height: parent.height * 0.2
                            radius: Theme.scaled(36)
                            color: scrubberArea.pressed && scrubberArea.velocityZone === -2 ? "#664CAF50" : "#22FFFFFF"
                            Text {
                                anchors.centerIn: parent
                                text: "\u25BC\u25BC"
                                color: "#666666"
                                font.pixelSize: Theme.scaled(16)
                            }
                        }
                    }
                }

                // Touch area for scrubbing
                MouseArea {
                    id: scrubberArea
                    anchors.fill: parent

                    property real startY: 0
                    property real startValue: 0
                    property int velocityZone: 0  // -2, -1, 0, 1, 2

                    onPressed: function(mouse) {
                        startY = mouse.y
                        startValue = root.value
                        velocityZone = 0
                        scrubTimer.start()
                    }

                    onReleased: {
                        scrubTimer.stop()
                        velocityZone = 0
                    }

                    onCanceled: {
                        scrubTimer.stop()
                        velocityZone = 0
                    }

                    onPositionChanged: function(mouse) {
                        // Calculate zone based on Y position relative to center
                        var centerY = height / 2
                        var offsetY = mouse.y - centerY
                        var zoneHeight = height / 5

                        if (Math.abs(offsetY) < zoneHeight * 0.75) {
                            velocityZone = 0  // Fine zone
                        } else if (offsetY < -zoneHeight * 1.5) {
                            velocityZone = 2  // Fast up
                        } else if (offsetY < 0) {
                            velocityZone = 1  // Medium up
                        } else if (offsetY > zoneHeight * 1.5) {
                            velocityZone = -2  // Fast down
                        } else {
                            velocityZone = -1  // Medium down
                        }
                    }

                    Timer {
                        id: scrubTimer
                        interval: 50
                        repeat: true
                        onTriggered: {
                            var multiplier = 0
                            switch (scrubberArea.velocityZone) {
                                case 2: multiplier = 10; break   // Fast up
                                case 1: multiplier = 3; break    // Medium up
                                case 0: multiplier = 0; break    // Fine (no auto)
                                case -1: multiplier = -3; break  // Medium down
                                case -2: multiplier = -10; break // Fast down
                            }
                            if (multiplier !== 0) {
                                adjustValue(multiplier)
                            }
                        }
                    }
                }

                // Fine adjustment with horizontal swipe in center
                MouseArea {
                    anchors.centerIn: parent
                    width: parent.width
                    height: Theme.scaled(120)

                    property real lastX: 0

                    onPressed: function(mouse) {
                        lastX = mouse.x
                    }

                    onPositionChanged: function(mouse) {
                        var deltaX = mouse.x - lastX
                        if (Math.abs(deltaX) > Theme.scaled(15)) {
                            adjustValue(deltaX > 0 ? 1 : -1)
                            lastX = mouse.x
                        }
                    }
                }
            }

            // Done button
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: Theme.scaled(60)
                width: Theme.scaled(160)
                height: Theme.scaled(56)
                radius: Theme.scaled(28)
                color: doneArea.pressed ? Qt.darker(root.accentColor, 1.2) : root.accentColor

                Text {
                    anchors.centerIn: parent
                    text: "Done"
                    font.pixelSize: Theme.scaled(18)
                    font.bold: true
                    color: "white"
                }

                MouseArea {
                    id: doneArea
                    anchors.fill: parent
                    onClicked: scrubberPopup.close()
                }
            }

            // Quick tap outside closes
            MouseArea {
                anchors.fill: parent
                z: -1
                onClicked: scrubberPopup.close()
            }
        }
    }

    function adjustValue(steps) {
        var newVal = root.value + (steps * root.stepSize)
        newVal = Math.max(root.from, Math.min(root.to, newVal))
        newVal = Math.round(newVal / root.stepSize) * root.stepSize
        if (newVal !== root.value) {
            root.valueModified(newVal)
        }
    }
}
