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

    // Shot comparison model
    property var comparisonModel: null

    // Visibility toggles for curve types
    property bool showPressure: true
    property bool showFlow: true
    property bool showTemperature: true
    property bool showWeight: true

    // Cursor state
    property double cursorTime: -1
    property bool cursorVisible: cursorTime >= 0

    // Colors for each shot (primary, flow/light, weight/lighter)
    readonly property var shotColorSets: [
        { primary: "#4CAF50", flow: "#81C784", weight: "#A5D6A7" },  // Green
        { primary: "#2196F3", flow: "#64B5F6", weight: "#90CAF9" },  // Blue
        { primary: "#FF9800", flow: "#FFB74D", weight: "#FFCC80" }   // Orange
    ]

    // Load data from comparison model
    function loadData() {
        if (!comparisonModel) return

        // Clear all series
        pressure1.clear(); flow1.clear(); temp1.clear(); weight1.clear()
        pressure2.clear(); flow2.clear(); temp2.clear(); weight2.clear()
        pressure3.clear(); flow3.clear(); temp3.clear(); weight3.clear()

        var windowStart = comparisonModel.windowStart

        // Load each shot's data with colors based on global position
        for (var i = 0; i < comparisonModel.shotCount; i++) {
            var pressureData = comparisonModel.getPressureData(i)
            var flowData = comparisonModel.getFlowData(i)
            var temperatureData = comparisonModel.getTemperatureData(i)
            var weightData = comparisonModel.getWeightData(i)

            var pSeries = i === 0 ? pressure1 : (i === 1 ? pressure2 : pressure3)
            var fSeries = i === 0 ? flow1 : (i === 1 ? flow2 : flow3)
            var tSeries = i === 0 ? temp1 : (i === 1 ? temp2 : temp3)
            var wSeries = i === 0 ? weight1 : (i === 1 ? weight2 : weight3)

            // Set colors based on global position (windowStart + i) % 3
            var colorIndex = (windowStart + i) % 3
            var colors = shotColorSets[colorIndex]
            pSeries.color = colors.primary
            fSeries.color = colors.flow
            tSeries.color = Theme.temperatureColor  // Use consistent temperature color
            wSeries.color = colors.weight

            for (var j = 0; j < pressureData.length; j++) {
                pSeries.append(pressureData[j].x, pressureData[j].y)
            }
            for (j = 0; j < flowData.length; j++) {
                fSeries.append(flowData[j].x, flowData[j].y)
            }
            for (j = 0; j < temperatureData.length; j++) {
                tSeries.append(temperatureData[j].x, temperatureData[j].y)
            }
            for (j = 0; j < weightData.length; j++) {
                wSeries.append(weightData[j].x, weightData[j].y / 5)  // Scale for display
            }
        }

        // Update axes - fit to data with small padding (minimum 15s for very short shots)
        timeAxis.max = Math.max(15, comparisonModel.maxTime + 0.5)
    }

    onComparisonModelChanged: loadData()
    Component.onCompleted: loadData()

    // Connect to model changes
    Connections {
        target: comparisonModel
        function onShotsChanged() { loadData() }
    }

    // Time axis
    ValueAxis {
        id: timeAxis
        min: 0
        max: 60
        tickCount: 7
        labelFormat: "%.0f"
        labelsColor: Theme.textSecondaryColor
        gridLineColor: Qt.rgba(255, 255, 255, 0.1)
        titleText: "Time (s)"
        titleBrush: Theme.textSecondaryColor
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

    // Weight axis (hidden, uses same right Y as temperature)
    ValueAxis {
        id: weightAxis
        min: 0
        max: 12
        visible: false
    }

    // Shot 1 series (Green)
    LineSeries {
        id: pressure1
        name: "Pressure 1"
        color: "#4CAF50"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow1
        name: "Flow 1"
        color: "#81C784"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight1
        name: "Weight 1"
        color: "#A5D6A7"
        width: Theme.graphLineWidth - 1
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: temp1
        name: "Temp 1"
        color: "#81C784"
        width: Theme.graphLineWidth - 1
        axisX: timeAxis
        axisYRight: tempAxis
        visible: chart.showTemperature
    }

    // Shot 2 series (Blue)
    LineSeries {
        id: pressure2
        name: "Pressure 2"
        color: "#2196F3"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow2
        name: "Flow 2"
        color: "#64B5F6"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight2
        name: "Weight 2"
        color: "#90CAF9"
        width: Theme.graphLineWidth - 1
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: temp2
        name: "Temp 2"
        color: "#64B5F6"
        width: Theme.graphLineWidth - 1
        axisX: timeAxis
        axisYRight: tempAxis
        visible: chart.showTemperature
    }

    // Shot 3 series (Orange)
    LineSeries {
        id: pressure3
        name: "Pressure 3"
        color: "#FF9800"
        width: Theme.graphLineWidth
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showPressure
    }
    LineSeries {
        id: flow3
        name: "Flow 3"
        color: "#FFB74D"
        width: Theme.graphLineWidth
        style: Qt.DashLine
        axisX: timeAxis
        axisY: pressureAxis
        visible: chart.showFlow
    }
    LineSeries {
        id: weight3
        name: "Weight 3"
        color: "#FFCC80"
        width: Theme.graphLineWidth - 1
        style: Qt.DotLine
        axisX: timeAxis
        axisY: weightAxis
        visible: chart.showWeight
    }
    LineSeries {
        id: temp3
        name: "Temp 3"
        color: "#FFB74D"
        width: Theme.graphLineWidth - 1
        axisX: timeAxis
        axisYRight: tempAxis
        visible: chart.showTemperature
    }

    // Handle click from external source (e.g., SwipeableArea)
    function handleClick(mouseX, mouseY) {
        var chartPos = chart.mapToValue(Qt.point(mouseX, mouseY), pressure1)
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

    // Interpolate value from LineSeries at given time
    function interpolateFromSeries(series, time) {
        if (!series || series.count === 0) return NaN
        var point0 = series.at(0)
        if (time <= point0.x) return point0.y
        var pointLast = series.at(series.count - 1)
        if (time >= pointLast.x) return pointLast.y
        for (var i = 1; i < series.count; i++) {
            var point = series.at(i)
            if (point.x >= time) {
                var prevPoint = series.at(i - 1)
                var t = (time - prevPoint.x) / (point.x - prevPoint.x)
                return prevPoint.y + t * (point.y - prevPoint.y)
            }
        }
        return NaN
    }

    // Click/tap handler
    MouseArea {
        anchors.fill: parent
        z: 100
        propagateComposedEvents: true
        onClicked: function(mouse) {
            var chartPos = chart.mapToValue(Qt.point(mouse.x, mouse.y), pressure1)
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
        z: 90
        x: {
            if (!cursorVisible) return 0
            var pos = chart.mapToPosition(Qt.point(cursorTime, 0), pressure1)
            return pos.x
        }
    }

    // Tooltip with data values for all shots
    Rectangle {
        id: cursorTooltip
        visible: cursorVisible
        color: Qt.rgba(0, 0, 0, 0.85)
        radius: 4
        width: tooltipColumn.width + 12
        height: tooltipColumn.height + 8
        border.color: Qt.rgba(1, 1, 1, 0.2)
        border.width: 1
        z: 95

        // Position near cursor, flipping side if too close to edge
        x: {
            if (!cursorVisible) return 0
            var pos = chart.mapToPosition(Qt.point(cursorTime, 0), pressure1)
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

            // Shot 1 values
            Column {
                visible: comparisonModel && comparisonModel.shotCount > 0
                spacing: 1

                Text {
                    text: comparisonModel ? "Shot " + (comparisonModel.windowStart + 1) : ""
                    color: comparisonModel ? shotColorSets[comparisonModel.windowStart % 3].primary : "white"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    visible: showPressure
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(pressure1, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " bar"
                    }
                    color: pressure1.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showFlow
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(flow1, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(2) + " mL/s"
                    }
                    color: flow1.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showTemperature
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(temp1, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " °C"
                    }
                    color: temp1.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showWeight
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(weight1, cursorTime) * 5
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " g"
                    }
                    color: weight1.color
                    font.pixelSize: 10
                }
            }

            // Shot 2 values
            Column {
                visible: comparisonModel && comparisonModel.shotCount > 1
                spacing: 1

                Text {
                    text: comparisonModel ? "Shot " + (comparisonModel.windowStart + 2) : ""
                    color: comparisonModel ? shotColorSets[(comparisonModel.windowStart + 1) % 3].primary : "white"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    visible: showPressure
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(pressure2, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " bar"
                    }
                    color: pressure2.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showFlow
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(flow2, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(2) + " mL/s"
                    }
                    color: flow2.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showTemperature
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(temp2, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " °C"
                    }
                    color: temp2.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showWeight
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(weight2, cursorTime) * 5
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " g"
                    }
                    color: weight2.color
                    font.pixelSize: 10
                }
            }

            // Shot 3 values
            Column {
                visible: comparisonModel && comparisonModel.shotCount > 2
                spacing: 1

                Text {
                    text: comparisonModel ? "Shot " + (comparisonModel.windowStart + 3) : ""
                    color: comparisonModel ? shotColorSets[(comparisonModel.windowStart + 2) % 3].primary : "white"
                    font.pixelSize: 10
                    font.bold: true
                }
                Text {
                    visible: showPressure
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(pressure3, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " bar"
                    }
                    color: pressure3.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showFlow
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(flow3, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(2) + " mL/s"
                    }
                    color: flow3.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showTemperature
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(temp3, cursorTime)
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " °C"
                    }
                    color: temp3.color
                    font.pixelSize: 10
                }
                Text {
                    visible: showWeight
                    text: {
                        if (!cursorVisible) return ""
                        var val = interpolateFromSeries(weight3, cursorTime) * 5
                        return isNaN(val) ? "" : "  " + val.toFixed(1) + " g"
                    }
                    color: weight3.color
                    font.pixelSize: 10
                }
            }
        }
    }
}
