import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DE1App
import "../components"

Page {
    id: profileEditorPage
    objectName: "profileEditorPage"
    background: Rectangle { color: Theme.backgroundColor }

    property var profile: null  // Profile being edited
    property int selectedStepIndex: -1

    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: Theme.surfaceColor

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15

            // Back button
            ActionButton {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 40
                text: "Back"
                backgroundColor: Theme.surfaceColor
                onClicked: root.goBack()
            }

            // Profile name
            TextField {
                id: profileNameField
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: profile ? profile.title : "New Profile"
                font: Theme.titleFont
                color: Theme.textColor
                background: Rectangle {
                    color: Qt.rgba(255, 255, 255, 0.1)
                    radius: 4
                }
            }

            // Mode toggle
            RowLayout {
                spacing: 5

                Text {
                    text: "Mode:"
                    color: Theme.textSecondaryColor
                    font: Theme.bodyFont
                }

                ComboBox {
                    id: modeCombo
                    Layout.preferredWidth: 150
                    model: ["Frame-Based", "Direct Control"]
                    currentIndex: profile && profile.mode === "direct" ? 1 : 0

                    background: Rectangle {
                        color: Theme.surfaceColor
                        border.color: Theme.accentColor
                        border.width: 1
                        radius: 4
                    }
                }
            }

            // Save button
            ActionButton {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 40
                text: "Save"
                backgroundColor: Theme.accentColor
                onClicked: saveProfile()
            }
        }
    }

    // Main content: Split view
    SplitView {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        orientation: Qt.Horizontal

        // Left panel: Step list
        Rectangle {
            SplitView.preferredWidth: 300
            SplitView.minimumWidth: 200
            color: Theme.surfaceColor

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                // Steps header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "Steps"
                        font: Theme.subtitleFont
                        color: Theme.textColor
                        Layout.fillWidth: true
                    }

                    ActionButton {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        text: "+"
                        backgroundColor: Theme.accentColor
                        onClicked: addStep()
                    }
                }

                // Step list
                ListView {
                    id: stepListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5
                    clip: true

                    model: profile ? profile.steps.length : 0

                    delegate: Rectangle {
                        width: stepListView.width
                        height: 80
                        radius: 8
                        color: index === selectedStepIndex ? Theme.accentColor :
                               Qt.rgba(255, 255, 255, 0.05)

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: (index + 1) + ". " + (profile.steps[index].name || "Step")
                                    font: Theme.bodyFont
                                    color: Theme.textColor
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                // Step type indicator
                                Rectangle {
                                    width: 50
                                    height: 20
                                    radius: 10
                                    color: profile.steps[index].pump === "flow" ?
                                           Theme.flowColor : Theme.pressureColor

                                    Text {
                                        anchors.centerIn: parent
                                        text: profile.steps[index].pump === "flow" ? "F" : "P"
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "white"
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15

                                Text {
                                    text: profile.steps[index].pump === "flow" ?
                                          profile.steps[index].flow.toFixed(1) + " mL/s" :
                                          profile.steps[index].pressure.toFixed(1) + " bar"
                                    font: Theme.captionFont
                                    color: Theme.textSecondaryColor
                                }

                                Text {
                                    text: profile.steps[index].temperature.toFixed(0) + "°C"
                                    font: Theme.captionFont
                                    color: Theme.temperatureColor
                                }

                                Text {
                                    text: profile.steps[index].seconds.toFixed(0) + "s"
                                    font: Theme.captionFont
                                    color: Theme.textSecondaryColor
                                }
                            }

                            // Exit condition indicator
                            Text {
                                visible: profile.steps[index].exit_if
                                text: "Exit: " + formatExitCondition(profile.steps[index])
                                font.pixelSize: 10
                                color: Theme.warningColor
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: selectedStepIndex = index
                        }
                    }
                }

                // Target settings
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    color: Qt.rgba(255, 255, 255, 0.05)
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        Text {
                            text: "Targets"
                            font: Theme.captionFont
                            color: Theme.textSecondaryColor
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 20

                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Weight"
                                    font.pixelSize: 10
                                    color: Theme.textSecondaryColor
                                }
                                SpinBox {
                                    from: 10
                                    to: 100
                                    value: profile ? profile.target_weight : 36
                                    stepSize: 1

                                    background: Rectangle {
                                        color: Theme.surfaceColor
                                        radius: 4
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Text {
                                    text: "Temp"
                                    font.pixelSize: 10
                                    color: Theme.textSecondaryColor
                                }
                                SpinBox {
                                    from: 70
                                    to: 100
                                    value: profile ? profile.espresso_temperature : 93

                                    background: Rectangle {
                                        color: Theme.surfaceColor
                                        radius: 4
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Right panel: Step editor
        Rectangle {
            SplitView.fillWidth: true
            color: Theme.backgroundColor

            Loader {
                anchors.fill: parent
                anchors.margins: 20
                sourceComponent: selectedStepIndex >= 0 ? stepEditorComponent : noSelectionComponent
            }
        }
    }

    // No selection placeholder
    Component {
        id: noSelectionComponent

        Item {
            Column {
                anchors.centerIn: parent
                spacing: 20

                Text {
                    text: "Select a step to edit"
                    font: Theme.titleFont
                    color: Theme.textSecondaryColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "Or click + to add a new step"
                    font: Theme.bodyFont
                    color: Theme.textSecondaryColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // Step editor component
    Component {
        id: stepEditorComponent

        ScrollView {
            id: stepEditorScroll

            property var step: profile && selectedStepIndex >= 0 ?
                              profile.steps[selectedStepIndex] : null

            ColumnLayout {
                width: stepEditorScroll.width - 20
                spacing: 20

                // Step name and actions
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15

                    TextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        text: step ? step.name : ""
                        placeholderText: "Step name"
                        font: Theme.titleFont
                        color: Theme.textColor

                        background: Rectangle {
                            color: Qt.rgba(255, 255, 255, 0.1)
                            radius: 4
                        }

                        onTextChanged: if (step) step.name = text
                    }

                    ActionButton {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 50
                        text: "Delete"
                        backgroundColor: Theme.errorColor
                        onClicked: deleteStep(selectedStepIndex)
                    }
                }

                // Control mode section
                GroupBox {
                    Layout.fillWidth: true
                    title: "Control Mode"

                    background: Rectangle {
                        color: Theme.surfaceColor
                        radius: 8
                        y: parent.topPadding - parent.padding
                        width: parent.width
                        height: parent.height - parent.topPadding + parent.padding
                    }

                    label: Text {
                        text: parent.title
                        font: Theme.subtitleFont
                        color: Theme.textColor
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 20

                        // Pump mode
                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: "Pump Mode"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            RowLayout {
                                spacing: 10

                                RadioButton {
                                    text: "Pressure"
                                    checked: step && step.pump === "pressure"
                                    onCheckedChanged: if (checked && step) step.pump = "pressure"
                                }

                                RadioButton {
                                    text: "Flow"
                                    checked: step && step.pump === "flow"
                                    onCheckedChanged: if (checked && step) step.pump = "flow"
                                }
                            }
                        }

                        // Setpoint value
                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: step && step.pump === "flow" ? "Flow (mL/s)" : "Pressure (bar)"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            Slider {
                                id: setpointSlider
                                Layout.preferredWidth: 200
                                from: 0
                                to: step && step.pump === "flow" ? 8 : 12
                                value: step ? (step.pump === "flow" ? step.flow : step.pressure) : 0
                                stepSize: 0.1

                                onValueChanged: {
                                    if (step) {
                                        if (step.pump === "flow") {
                                            step.flow = value
                                        } else {
                                            step.pressure = value
                                        }
                                    }
                                }
                            }

                            Text {
                                text: setpointSlider.value.toFixed(1)
                                font: Theme.bodyFont
                                color: step && step.pump === "flow" ?
                                       Theme.flowColor : Theme.pressureColor
                            }
                        }

                        // Temperature
                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: "Temperature (°C)"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            Slider {
                                id: tempSlider
                                Layout.preferredWidth: 150
                                from: 70
                                to: 100
                                value: step ? step.temperature : 93
                                stepSize: 0.5

                                onValueChanged: if (step) step.temperature = value
                            }

                            Text {
                                text: tempSlider.value.toFixed(1)
                                font: Theme.bodyFont
                                color: Theme.temperatureColor
                            }
                        }

                        // Duration
                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: "Duration (s)"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            SpinBox {
                                from: 1
                                to: 120
                                value: step ? step.seconds : 30
                                stepSize: 1

                                onValueChanged: if (step) step.seconds = value
                            }
                        }
                    }
                }

                // Transition section
                GroupBox {
                    Layout.fillWidth: true
                    title: "Transition"

                    background: Rectangle {
                        color: Theme.surfaceColor
                        radius: 8
                        y: parent.topPadding - parent.padding
                        width: parent.width
                        height: parent.height - parent.topPadding + parent.padding
                    }

                    label: Text {
                        text: parent.title
                        font: Theme.subtitleFont
                        color: Theme.textColor
                    }

                    RowLayout {
                        spacing: 30

                        RadioButton {
                            text: "Fast (instant)"
                            checked: step && step.transition === "fast"
                            onCheckedChanged: if (checked && step) step.transition = "fast"
                        }

                        RadioButton {
                            text: "Smooth (interpolate)"
                            checked: step && step.transition === "smooth"
                            onCheckedChanged: if (checked && step) step.transition = "smooth"
                        }

                        ColumnLayout {
                            spacing: 4

                            Text {
                                text: "Sensor"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            ComboBox {
                                model: ["Coffee (basket)", "Water (mix)"]
                                currentIndex: step && step.sensor === "water" ? 1 : 0

                                onCurrentIndexChanged: {
                                    if (step) step.sensor = currentIndex === 1 ? "water" : "coffee"
                                }
                            }
                        }
                    }
                }

                // Exit conditions
                GroupBox {
                    Layout.fillWidth: true
                    title: "Exit Conditions"

                    background: Rectangle {
                        color: Theme.surfaceColor
                        radius: 8
                        y: parent.topPadding - parent.padding
                        width: parent.width
                        height: parent.height - parent.topPadding + parent.padding
                    }

                    label: Text {
                        text: parent.title
                        font: Theme.subtitleFont
                        color: Theme.textColor
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15

                        CheckBox {
                            id: exitIfCheck
                            text: "Enable early exit condition"
                            checked: step ? step.exit_if : false
                            onCheckedChanged: if (step) step.exit_if = checked
                        }

                        GridLayout {
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 30
                            enabled: exitIfCheck.checked

                            RadioButton {
                                text: "Pressure Over"
                                checked: step && step.exit_type === "pressure_over"
                                onCheckedChanged: if (checked && step) step.exit_type = "pressure_over"
                            }

                            RowLayout {
                                spacing: 10
                                enabled: step && step.exit_type === "pressure_over"

                                Slider {
                                    Layout.preferredWidth: 150
                                    from: 0
                                    to: 12
                                    value: step ? step.exit_pressure_over : 0
                                    stepSize: 0.1
                                    onValueChanged: if (step) step.exit_pressure_over = value
                                }

                                Text {
                                    text: (step ? step.exit_pressure_over : 0).toFixed(1) + " bar"
                                    font: Theme.bodyFont
                                    color: Theme.pressureColor
                                }
                            }

                            RadioButton {
                                text: "Pressure Under"
                                checked: step && step.exit_type === "pressure_under"
                                onCheckedChanged: if (checked && step) step.exit_type = "pressure_under"
                            }

                            RowLayout {
                                spacing: 10
                                enabled: step && step.exit_type === "pressure_under"

                                Slider {
                                    Layout.preferredWidth: 150
                                    from: 0
                                    to: 12
                                    value: step ? step.exit_pressure_under : 0
                                    stepSize: 0.1
                                    onValueChanged: if (step) step.exit_pressure_under = value
                                }

                                Text {
                                    text: (step ? step.exit_pressure_under : 0).toFixed(1) + " bar"
                                    font: Theme.bodyFont
                                    color: Theme.pressureColor
                                }
                            }

                            RadioButton {
                                text: "Flow Over"
                                checked: step && step.exit_type === "flow_over"
                                onCheckedChanged: if (checked && step) step.exit_type = "flow_over"
                            }

                            RowLayout {
                                spacing: 10
                                enabled: step && step.exit_type === "flow_over"

                                Slider {
                                    Layout.preferredWidth: 150
                                    from: 0
                                    to: 8
                                    value: step ? step.exit_flow_over : 0
                                    stepSize: 0.1
                                    onValueChanged: if (step) step.exit_flow_over = value
                                }

                                Text {
                                    text: (step ? step.exit_flow_over : 0).toFixed(1) + " mL/s"
                                    font: Theme.bodyFont
                                    color: Theme.flowColor
                                }
                            }

                            RadioButton {
                                text: "Flow Under"
                                checked: step && step.exit_type === "flow_under"
                                onCheckedChanged: if (checked && step) step.exit_type = "flow_under"
                            }

                            RowLayout {
                                spacing: 10
                                enabled: step && step.exit_type === "flow_under"

                                Slider {
                                    Layout.preferredWidth: 150
                                    from: 0
                                    to: 8
                                    value: step ? step.exit_flow_under : 0
                                    stepSize: 0.1
                                    onValueChanged: if (step) step.exit_flow_under = value
                                }

                                Text {
                                    text: (step ? step.exit_flow_under : 0).toFixed(1) + " mL/s"
                                    font: Theme.bodyFont
                                    color: Theme.flowColor
                                }
                            }
                        }
                    }
                }

                // Limiter section
                GroupBox {
                    Layout.fillWidth: true
                    title: "Limiter (optional)"

                    background: Rectangle {
                        color: Theme.surfaceColor
                        radius: 8
                        y: parent.topPadding - parent.padding
                        width: parent.width
                        height: parent.height - parent.topPadding + parent.padding
                    }

                    label: Text {
                        text: parent.title
                        font: Theme.subtitleFont
                        color: Theme.textColor
                    }

                    RowLayout {
                        spacing: 30

                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: step && step.pump === "flow" ?
                                      "Max Pressure" : "Max Flow"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            Slider {
                                id: limiterSlider
                                Layout.preferredWidth: 200
                                from: 0
                                to: step && step.pump === "flow" ? 12 : 8
                                value: step ? step.max_flow_or_pressure : 0
                                stepSize: 0.1

                                onValueChanged: if (step) step.max_flow_or_pressure = value
                            }

                            Text {
                                text: limiterSlider.value > 0 ?
                                      limiterSlider.value.toFixed(1) +
                                      (step && step.pump === "flow" ? " bar" : " mL/s") :
                                      "Disabled"
                                font: Theme.bodyFont
                                color: limiterSlider.value > 0 ?
                                       Theme.warningColor : Theme.textSecondaryColor
                            }
                        }

                        ColumnLayout {
                            spacing: 8
                            enabled: limiterSlider.value > 0

                            Text {
                                text: "Limiter Range"
                                font: Theme.captionFont
                                color: Theme.textSecondaryColor
                            }

                            Slider {
                                Layout.preferredWidth: 150
                                from: 0.1
                                to: 2.0
                                value: step ? step.max_flow_or_pressure_range : 0.6
                                stepSize: 0.1

                                onValueChanged: if (step) step.max_flow_or_pressure_range = value
                            }

                            Text {
                                text: (step ? step.max_flow_or_pressure_range : 0.6).toFixed(1)
                                font: Theme.bodyFont
                                color: Theme.textSecondaryColor
                            }
                        }
                    }
                }

                // Spacer
                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // Helper functions
    function formatExitCondition(step) {
        if (!step.exit_if) return ""

        switch (step.exit_type) {
            case "pressure_over":
                return "P > " + step.exit_pressure_over.toFixed(1) + " bar"
            case "pressure_under":
                return "P < " + step.exit_pressure_under.toFixed(1) + " bar"
            case "flow_over":
                return "F > " + step.exit_flow_over.toFixed(1) + " mL/s"
            case "flow_under":
                return "F < " + step.exit_flow_under.toFixed(1) + " mL/s"
            default:
                return ""
        }
    }

    function addStep() {
        if (!profile) return

        var newStep = {
            name: "Step " + (profile.steps.length + 1),
            temperature: 93.0,
            sensor: "coffee",
            pump: "pressure",
            transition: "fast",
            pressure: 9.0,
            flow: 2.0,
            seconds: 30.0,
            volume: 0,
            exit_if: false,
            exit_type: "",
            exit_pressure_over: 0,
            exit_pressure_under: 0,
            exit_flow_over: 0,
            exit_flow_under: 0,
            max_flow_or_pressure: 0,
            max_flow_or_pressure_range: 0.6
        }

        profile.steps.push(newStep)
        selectedStepIndex = profile.steps.length - 1
        stepListView.model = profile.steps.length
    }

    function deleteStep(index) {
        if (!profile || index < 0 || index >= profile.steps.length) return

        profile.steps.splice(index, 1)

        if (selectedStepIndex >= profile.steps.length) {
            selectedStepIndex = profile.steps.length - 1
        }

        stepListView.model = profile.steps.length
    }

    function saveProfile() {
        // Update profile name
        if (profile) {
            profile.title = profileNameField.text
            profile.mode = modeCombo.currentIndex === 1 ? "direct" : "frame_based"
            // Save to file via MainController
            // MainController.saveProfile(profile)
            console.log("Profile saved:", JSON.stringify(profile, null, 2))
        }
    }

    Component.onCompleted: {
        // Load current profile if not set
        if (!profile) {
            // Load default or create new
            profile = {
                title: "New Profile",
                steps: [],
                target_weight: 36,
                espresso_temperature: 93,
                mode: "frame_based"
            }
        }
    }
}
