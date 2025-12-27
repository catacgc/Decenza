import QtQuick
import QtQuick.Layouts
import DecenzaDE1

Item {
    id: root

    property var presets: []
    property int selectedIndex: -1
    property real maxWidth: Theme.scaled(900)  // Max width before wrapping

    signal presetSelected(int index)

    implicitHeight: contentColumn.implicitHeight
    implicitWidth: maxWidth

    // Calculate how many pills fit per row
    readonly property real pillSpacing: Theme.scaled(12)
    readonly property real pillPadding: Theme.scaled(40)  // Horizontal padding inside pill

    // Hidden TextMetrics for measuring pill text widths
    TextMetrics {
        id: textMetrics
        font.pixelSize: Theme.scaled(16)
        font.bold: true
    }

    function measureTextWidth(text) {
        textMetrics.text = text
        return textMetrics.width
    }

    // Group presets into rows, distributing evenly for aesthetics (3/2 instead of 4/1)
    function calculateRows() {
        if (presets.length === 0) return []

        // First pass: calculate pill widths based on actual text width
        var pillWidths = []
        for (var i = 0; i < presets.length; i++) {
            var textWidth = measureTextWidth(presets[i].name || "")
            pillWidths.push(textWidth + pillPadding)
        }

        // Count how many rows we need with greedy packing
        var numRows = 1
        var rowWidth = 0
        for (i = 0; i < presets.length; i++) {
            var neededWidth = rowWidth > 0 ? pillWidths[i] + pillSpacing : pillWidths[i]
            if (rowWidth + neededWidth > maxWidth) {
                numRows++
                rowWidth = pillWidths[i]
            } else {
                rowWidth += neededWidth
            }
        }

        // If only one row needed, just return all in one row
        if (numRows === 1) {
            var singleRow = []
            for (i = 0; i < presets.length; i++) {
                singleRow.push({index: i, preset: presets[i], width: pillWidths[i]})
            }
            return [singleRow]
        }

        // Distribute pills evenly across rows
        var pillsPerRow = Math.ceil(presets.length / numRows)
        var rows = []
        var currentRow = []

        for (i = 0; i < presets.length; i++) {
            currentRow.push({index: i, preset: presets[i], width: pillWidths[i]})

            if (currentRow.length >= pillsPerRow) {
                rows.push(currentRow)
                currentRow = []
                // Recalculate for remaining pills to keep distribution even
                var remaining = presets.length - i - 1
                var remainingRows = numRows - rows.length
                if (remainingRows > 0) {
                    pillsPerRow = Math.ceil(remaining / remainingRows)
                }
            }
        }

        if (currentRow.length > 0) {
            rows.push(currentRow)
        }

        return rows
    }

    Column {
        id: contentColumn
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: Theme.scaled(8)

        Repeater {
            model: root.calculateRows()

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: root.pillSpacing

                Repeater {
                    model: modelData

                    Rectangle {
                        id: pill

                        property bool isSelected: modelData.index === root.selectedIndex

                        width: pillText.implicitWidth + root.pillPadding
                        height: Theme.scaled(50)
                        radius: Theme.scaled(10)

                        color: isSelected ? Theme.primaryColor : Theme.backgroundColor
                        border.color: isSelected ? Theme.primaryColor : Theme.textSecondaryColor
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        Text {
                            id: pillText
                            anchors.centerIn: parent
                            text: modelData.preset.name || ""
                            color: pill.isSelected ? "white" : Theme.textColor
                            font.pixelSize: Theme.scaled(16)
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.presetSelected(modelData.index)
                        }
                    }
                }
            }
        }
    }
}
