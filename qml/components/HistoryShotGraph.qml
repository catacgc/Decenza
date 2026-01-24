import QtQuick
import QtCharts
import DecenzaDE1

ChartView {
    id: chart
    antialiasing: true
    backgroundColor: "transparent"
    plotAreaColor: Qt.darker(Theme.surfaceColor, 1.3)
    legend.visible: false

    margins.top: 0
    margins.bottom: 0
    margins.left: 0
    margins.right: 0

    // Data to display (set from parent)
    property var pressureData: []
    property var flowData: []
    property var temperatureData: []
    property var weightData: []
    property var phaseMarkers: []
    property double maxTime: 60

    // Cursor state
    property double cursorTime: -1
    property bool cursorVisible: cursorTime >= 0

    // Load data into series
    function loadData() {
        pressureSeries.clear()
        flowSeries.clear()
        temperatureSeries.clear()
        weightSeries.clear()

        for (var i = 0; i < pressureData.length; i++) {
            pressureSeries.append(pressureData[i].x, pressureData[i].y)
        }
        for (i = 0; i < flowData.length; i++) {
            flowSeries.append(flowData[i].x, flowData[i].y)
        }
        for (i = 0; i < temperatureData.length; i++) {
            temperatureSeries.append(temperatureData[i].x, temperatureData[i].y)
        }
        for (i = 0; i < weightData.length; i++) {
            weightSeries.append(weightData[i].x, weightData[i].y)
        }

        // Update time axis
        if (pressureData.length > 0) {
            timeAxis.max = Math.max(5, maxTime + 2)
        }
    }

    onPressureDataChanged: { cursorTime = -1; loadData() }
    Component.onCompleted: loadData()

    // Time axis
    ValueAxis {
        id: timeAxis
        min: 0
        max: 60
        tickCount: 7
        labelFormat: "%.0f"
        labelsColor: Theme.textSecondaryColor
        gridLineColor: Qt.rgba(255, 255, 255, 0.1)
    }

    // Pressure/Flow axis (left Y)
    ValueAxis {
        id: pressureAxis
        min: 0
        max: 12
        tickCount: 5
        labelFormat: "%.0f"
        labelsColor: Theme.textSecondaryColor
        gridLineColor: Qt.rgba(255, 255, 255, 0.1)
        titleText: "bar / mL/s"
        titleBrush: Theme.textSecondaryColor
    }

    // Temperature axis (right Y) - hidden to make room for weight
    ValueAxis {
        id: tempAxis
        min: 80
        max: 100
        tickCount: 5
        labelFormat: "%.0f"
        labelsColor: Theme.temperatureColor
        gridLineColor: "transparent"
        titleText: "°C"
        titleBrush: Theme.temperatureColor
        visible: false
    }

    // Weight axis (right Y) - scaled to max weight in data + 10%
    property double maxWeight: {
        var max = 0
        for (var i = 0; i < weightData.length; i++) {
            if (weightData[i].y > max) max = weightData[i].y
        }
        return Math.max(10, max * 1.1)
    }

    ValueAxis {
        id: weightAxis
        min: 0
        max: maxWeight
        tickCount: 5
        labelFormat: "%.0f"
        labelsColor: Theme.weightColor
        gridLineColor: "transparent"
        titleText: "g"
        titleBrush: Theme.weightColor
    }

    // Pressure line
    LineSeries {
        id: pressureSeries
        name: "Pressure"
        color: Theme.pressureColor
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
    }

    // Flow line
    LineSeries {
        id: flowSeries
        name: "Flow"
        color: Theme.flowColor
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
    }

    // Temperature line
    LineSeries {
        id: temperatureSeries
        name: "Temperature"
        color: Theme.temperatureColor
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisYRight: tempAxis
    }

    // Weight line
    LineSeries {
        id: weightSeries
        name: "Weight"
        color: Theme.weightColor
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisYRight: weightAxis
    }

    // Handle click from external source (e.g., SwipeableArea)
    function handleClick(mouseX, mouseY) {
        var chartPos = chart.mapToValue(Qt.point(mouseX, mouseY), pressureSeries)
        if (chartPos.x >= timeAxis.min && chartPos.x <= timeAxis.max) {
            chart.cursorTime = chartPos.x
        } else {
            chart.cursorTime = -1
        }
    }

    // Clear cursor (e.g., on press-and-hold)
    function clearCursor() {
        chart.cursorTime = -1
    }

    // Interpolate value from data array at given time
    function interpolateAt(data, time) {
        if (!data || data.length === 0) return NaN
        if (time <= data[0].x) return data[0].y
        if (time >= data[data.length - 1].x) return data[data.length - 1].y
        for (var i = 1; i < data.length; i++) {
            if (data[i].x >= time) {
                var t = (time - data[i-1].x) / (data[i].x - data[i-1].x)
                return data[i-1].y + t * (data[i].y - data[i-1].y)
            }
        }
        return NaN
    }

    // Click/tap handler
    MouseArea {
        anchors.fill: parent
        onClicked: function(mouse) {
            var chartPos = chart.mapToValue(Qt.point(mouse.x, mouse.y), pressureSeries)
            if (chartPos.x >= timeAxis.min && chartPos.x <= timeAxis.max) {
                chart.cursorTime = chartPos.x
            } else {
                chart.cursorTime = -1
            }
        }
        onPressAndHold: {
            chart.cursorTime = -1
        }
    }

    // Vertical cursor line
    Rectangle {
        id: cursorLine
        visible: cursorVisible
        width: 1
        color: Theme.textColor
        opacity: 0.7
        y: chart.plotArea.y
        height: chart.plotArea.height
        x: {
            if (!cursorVisible) return 0
            var pos = chart.mapToPosition(Qt.point(cursorTime, 0), pressureSeries)
            return pos.x
        }
    }

    // Tooltip with data values
    Rectangle {
        id: cursorTooltip
        visible: cursorVisible
        color: Qt.rgba(0, 0, 0, 0.85)
        radius: 4
        width: tooltipColumn.width + 12
        height: tooltipColumn.height + 8
        border.color: Qt.rgba(1, 1, 1, 0.2)
        border.width: 1

        // Position near cursor, flipping side if too close to edge
        x: {
            if (!cursorVisible) return 0
            var pos = chart.mapToPosition(Qt.point(cursorTime, 0), pressureSeries)
            var tooltipX = pos.x + 8
            if (tooltipX + width > chart.plotArea.x + chart.plotArea.width)
                tooltipX = pos.x - width - 8
            return tooltipX
        }
        y: chart.plotArea.y + 8

        Column {
            id: tooltipColumn
            x: 6
            y: 4
            spacing: 2

            Text {
                text: cursorVisible ? cursorTime.toFixed(1) + "s" : ""
                color: Theme.textColor
                font.pixelSize: 11
                font.bold: true
            }
            Text {
                text: cursorVisible ? interpolateAt(pressureData, cursorTime).toFixed(1) + " bar" : ""
                color: Theme.pressureColor
                font.pixelSize: 11
            }
            Text {
                text: cursorVisible ? interpolateAt(flowData, cursorTime).toFixed(2) + " mL/s" : ""
                color: Theme.flowColor
                font.pixelSize: 11
            }
            Text {
                text: cursorVisible ? interpolateAt(temperatureData, cursorTime).toFixed(1) + " °C" : ""
                color: Theme.temperatureColor
                font.pixelSize: 11
            }
            Text {
                text: cursorVisible ? interpolateAt(weightData, cursorTime).toFixed(1) + " g" : ""
                color: Theme.weightColor
                font.pixelSize: 11
            }
        }
    }
}
