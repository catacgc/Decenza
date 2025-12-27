# Phase 2: Screen Reader Support

## Goal
Add `Accessible.*` properties to all interactive components so screen readers (TalkBack on Android, VoiceOver on iOS) can identify and describe UI elements.

## Prerequisites
- Phase 1 completed (AccessibilityManager exists)

---

## Overview

Every interactive element needs these properties:
- `Accessible.role`: What kind of control is it? (Button, Slider, etc.)
- `Accessible.name`: What does it do? (spoken by screen reader)
- `Accessible.description`: How to use it? (optional additional context)
- `Accessible.focusable: true`: Can receive keyboard/screen reader focus

---

## Step 1: Update ActionButton.qml

**File: `qml/components/ActionButton.qml`**

Add accessibility properties inside the Button:

```qml
Button {
    id: control

    property string iconSource: ""
    property color backgroundColor: Theme.primaryColor

    // Accessibility
    Accessible.role: Accessible.Button
    Accessible.name: control.text
    Accessible.description: "Double-click to select profile. Long-press for settings."
    Accessible.focusable: true

    // ... rest of existing code
}
```

---

## Step 2: Update ValueInput.qml

**File: `qml/components/ValueInput.qml`**

Add accessibility properties to the root Item and interactive sub-components:

```qml
Item {
    id: root

    // ... existing properties ...

    // Accessibility - expose as a slider
    Accessible.role: Accessible.Slider
    Accessible.name: {
        var label = root.displayText || (root.value.toFixed(root.decimals) + root.suffix)
        return label
    }
    Accessible.description: "Use plus and minus buttons to adjust. Tap center for full-screen editor."
    Accessible.focusable: true

    // For value announcement when changed
    onValueChanged: {
        if (AccessibilityManager.enabled) {
            AccessibilityManager.announce(root.value.toFixed(root.decimals) + " " + root.suffix.trim())
        }
    }

    // ... existing code ...

    // Update minus button Rectangle to have accessible name
    Rectangle {
        // ... existing minus button code ...

        Accessible.role: Accessible.Button
        Accessible.name: "Decrease"
        Accessible.focusable: true
    }

    // Update plus button Rectangle to have accessible name
    Rectangle {
        // ... existing plus button code ...

        Accessible.role: Accessible.Button
        Accessible.name: "Increase"
        Accessible.focusable: true
    }
}
```

**Important**: The popup also needs accessibility:

```qml
Popup {
    id: scrubberPopup
    // ... existing code ...

    Accessible.role: Accessible.Dialog
    Accessible.name: "Value editor"

    // On open, announce
    onOpened: {
        if (AccessibilityManager.enabled) {
            AccessibilityManager.announce("Value editor. Current value: " + root.value.toFixed(root.decimals) + " " + root.suffix.trim(), true)
        }
    }
}
```

---

## Step 3: Update TouchSlider.qml (if exists)

**File: `qml/components/TouchSlider.qml`**

Similar pattern to ValueInput:

```qml
Item {
    id: root

    Accessible.role: Accessible.Slider
    Accessible.name: root.value.toFixed(root.decimals) + " " + root.suffix
    Accessible.description: "Slide or use plus minus buttons to adjust"
    Accessible.focusable: true

    // Announce value changes
    onValueChanged: {
        if (AccessibilityManager.enabled) {
            AccessibilityManager.announce(root.value.toFixed(root.decimals) + " " + root.suffix.trim())
        }
    }
}
```

---

## Step 4: Update PresetPillRow.qml

**File: `qml/components/PresetPillRow.qml`**

Each pill button needs accessibility. Find the Repeater delegate and add:

```qml
// Inside the pill Rectangle delegate
Rectangle {
    id: pill
    // ... existing code ...

    Accessible.role: Accessible.Button
    Accessible.name: modelData.name + (isSelected ? ", selected" : "")
    Accessible.description: "Double-click to edit. Drag to reorder."
    Accessible.focusable: true
}
```

---

## Step 5: Update StatusBar.qml

**File: `qml/components/StatusBar.qml`**

Add accessibility to status indicators:

```qml
// For each status item (temperature, water level, connection, etc.)
Item {
    Accessible.role: Accessible.StaticText
    Accessible.name: {
        // Example for temperature
        return "Temperature: " + temperatureValue + " degrees"
    }
}

// For connection indicator
Rectangle {
    id: connectionIndicator
    // ... existing code ...

    Accessible.role: Accessible.Indicator
    Accessible.name: DE1Device.connected ? "Machine connected" : "Machine disconnected"
}
```

---

## Step 6: Update BottomBar.qml

**File: `qml/components/BottomBar.qml`**

Add accessibility to the back button:

```qml
// Find the back button Image/MouseArea and wrap or add:
Item {
    id: backButton
    // ... existing code ...

    Accessible.role: Accessible.Button
    Accessible.name: "Back"
    Accessible.description: "Go back to previous screen"
    Accessible.focusable: true
}
```

---

## Step 7: Update IdlePage.qml

**File: `qml/pages/IdlePage.qml`**

Each ActionButton should have descriptive accessibility:

```qml
ActionButton {
    id: espressoButton
    text: "Espresso"
    iconSource: "qrc:/icons/espresso.svg"

    // Override generic description with specific one
    Accessible.description: "Start espresso. Double-click to select profile. Long-press for settings."
}

ActionButton {
    id: steamButton
    text: "Steam"
    iconSource: "qrc:/icons/steam.svg"

    Accessible.description: "Start steaming milk. Long-press to configure."
}

ActionButton {
    id: hotWaterButton
    text: "Hot Water"
    iconSource: "qrc:/icons/hotwater.svg"

    Accessible.description: "Dispense hot water. Long-press to configure."
}

ActionButton {
    id: flushButton
    text: "Flush"
    iconSource: "qrc:/icons/flush.svg"

    Accessible.description: "Flush the group head. Long-press to configure."
}

// Sleep button
Button {
    id: sleepButton
    // ... existing code ...

    Accessible.role: Accessible.Button
    Accessible.name: "Sleep"
    Accessible.description: "Put the machine to sleep"
    Accessible.focusable: true
}

// Settings button
Button {
    id: settingsButton
    // ... existing code ...

    Accessible.role: Accessible.Button
    Accessible.name: "Settings"
    Accessible.description: "Open application settings"
    Accessible.focusable: true
}
```

---

## Step 8: Update SteamPage.qml (and similar pages)

**File: `qml/pages/SteamPage.qml`** (also HotWaterPage.qml, FlushPage.qml)

Add accessibility to page-level elements:

```qml
Page {
    // Page-level accessibility
    Accessible.role: Accessible.Pane
    Accessible.name: "Steam settings"

    // For preset pills area
    Item {
        Accessible.role: Accessible.ToolBar
        Accessible.name: "Pitcher presets"
    }

    // For value controls (duration, flow, temp)
    // These should inherit from ValueInput accessibility
}
```

---

## Step 9: Update EspressoPage.qml

**File: `qml/pages/EspressoPage.qml`**

The extraction page needs special attention:

```qml
Page {
    id: espressoPage

    Accessible.role: Accessible.Pane
    Accessible.name: "Espresso extraction in progress"

    // Preheating banner
    Rectangle {
        id: statusBanner
        // ... existing code ...

        Accessible.role: Accessible.Alert
        Accessible.name: "Preheating"
    }

    // Info bar items - each value display needs accessibility
    // Timer
    ColumnLayout {
        Accessible.role: Accessible.StaticText
        Accessible.name: "Time: " + MachineState.shotTime.toFixed(1) + " seconds"
    }

    // Pressure
    ColumnLayout {
        Accessible.role: Accessible.StaticText
        Accessible.name: "Pressure: " + DE1Device.pressure.toFixed(1) + " bar"
    }

    // Flow
    ColumnLayout {
        Accessible.role: Accessible.StaticText
        Accessible.name: "Flow: " + DE1Device.flow.toFixed(1) + " milliliters per second"
    }

    // Temperature
    ColumnLayout {
        Accessible.role: Accessible.StaticText
        Accessible.name: "Temperature: " + DE1Device.temperature.toFixed(1) + " degrees"
    }

    // Weight
    ColumnLayout {
        Accessible.role: Accessible.StaticText
        Accessible.name: "Weight: " + espressoPage.currentWeight.toFixed(1) + " of " + MainController.targetWeight.toFixed(0) + " grams"
    }

    // Back/stop button
    MouseArea {
        // ... existing code ...

        Accessible.role: Accessible.Button
        Accessible.name: "Stop and go back"
        Accessible.description: "Tap to stop extraction and return to home"
        Accessible.focusable: true
    }
}
```

---

## Step 10: Update SettingsPage.qml

**File: `qml/pages/SettingsPage.qml`**

Add accessibility to tabs and settings controls:

```qml
// TabBar
TabBar {
    Accessible.role: Accessible.TabList
    Accessible.name: "Settings tabs"
}

// Each TabButton
TabButton {
    Accessible.role: Accessible.Tab
    Accessible.name: text  // "Bluetooth", "Preferences", etc.
}

// Switches
Switch {
    Accessible.role: Accessible.CheckBox
    Accessible.name: "USB Charger"
    Accessible.checked: checked
}

// ListViews
ListView {
    Accessible.role: Accessible.List
    Accessible.name: "Discovered devices"
}
```

---

## Step 11: Update Dialogs in main.qml

**File: `qml/main.qml`**

Each dialog needs accessibility:

```qml
Dialog {
    id: bleErrorDialog
    // ... existing code ...

    Accessible.role: Accessible.AlertMessage
    Accessible.name: title + ". " + contentText

    onOpened: {
        if (AccessibilityManager.enabled) {
            AccessibilityManager.announce(title + ". " + contentText, true)
        }
    }
}
```

Apply to all dialogs:
- `bleErrorDialog`
- `flowScaleDialog`
- `scaleDisconnectedDialog`
- `refillDialog`
- `firstRunDialog`

---

## Verification Checklist

After implementing Phase 2:

- [ ] Enable TalkBack on Android device
- [ ] Navigate to each page using screen reader gestures
- [ ] All buttons are announced with their names
- [ ] Sliders announce their current value
- [ ] Status indicators announce their state
- [ ] Dialogs are announced when they open
- [ ] Preset pills announce selected state
- [ ] Back buttons are identified

---

## Files Modified

| File | Action |
|------|--------|
| qml/components/ActionButton.qml | MODIFY - add Accessible properties |
| qml/components/ValueInput.qml | MODIFY - add Accessible properties |
| qml/components/TouchSlider.qml | MODIFY - add Accessible properties |
| qml/components/PresetPillRow.qml | MODIFY - add Accessible properties |
| qml/components/StatusBar.qml | MODIFY - add Accessible properties |
| qml/components/BottomBar.qml | MODIFY - add Accessible properties |
| qml/pages/IdlePage.qml | MODIFY - add Accessible descriptions |
| qml/pages/EspressoPage.qml | MODIFY - add Accessible properties to info bar |
| qml/pages/SteamPage.qml | MODIFY - add Accessible properties |
| qml/pages/HotWaterPage.qml | MODIFY - add Accessible properties |
| qml/pages/FlushPage.qml | MODIFY - add Accessible properties |
| qml/pages/SettingsPage.qml | MODIFY - add Accessible properties |
| qml/main.qml | MODIFY - add Accessible properties to dialogs |

---

## Notes for Implementation

1. **Accessible.name should be dynamic**: For elements that display changing values (pressure, weight, time), the Accessible.name should use bindings to update automatically.

2. **Don't over-announce**: The value change announcements in ValueInput should be throttled or only spoken when the user finishes adjusting (on release), not during drag.

3. **Testing with TalkBack**:
   - Swipe right to move to next element
   - Swipe left to move to previous element
   - Double-tap to activate
   - Explore by touch to hear what's under your finger

4. **Qt Accessible roles**: Common roles include:
   - `Accessible.Button`
   - `Accessible.Slider`
   - `Accessible.CheckBox`
   - `Accessible.StaticText`
   - `Accessible.Dialog`
   - `Accessible.Alert`
   - `Accessible.List`
   - `Accessible.ListItem`
   - `Accessible.Tab`
   - `Accessible.TabList`
   - `Accessible.Pane`
