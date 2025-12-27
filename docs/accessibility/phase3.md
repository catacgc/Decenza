# Phase 3: Keyboard Navigation

## Goal
Add keyboard navigation support so users can navigate the app using Tab, arrow keys, and Enter without touch. This also enables screen reader navigation on Android/iOS.

## Prerequisites
- Phase 1 completed (AccessibilityManager exists)
- Phase 2 completed (Accessible properties added)

---

## Overview

Keyboard navigation requires:
1. **Focus indicators**: Visual feedback showing which element is focused
2. **KeyNavigation chains**: Define navigation order between elements
3. **Key handlers**: Respond to Enter, Space, Arrow keys
4. **activeFocusOnTab**: Allow Tab key to focus elements

---

## Step 1: Create FocusIndicator Component

**File: `qml/components/FocusIndicator.qml`** (NEW FILE)

```qml
import QtQuick

Rectangle {
    id: focusIndicator

    // Parent must have focus-related properties
    property Item targetItem: parent

    anchors.fill: parent
    anchors.margins: -2

    visible: targetItem.activeFocus
    color: "transparent"
    border.width: 3
    border.color: Theme.primaryColor
    radius: parent.radius ? parent.radius + 2 : 4

    // Pulsing animation when focused
    SequentialAnimation on border.color {
        running: focusIndicator.visible
        loops: Animation.Infinite

        ColorAnimation {
            to: Qt.lighter(Theme.primaryColor, 1.3)
            duration: 500
        }
        ColorAnimation {
            to: Theme.primaryColor
            duration: 500
        }
    }
}
```

---

## Step 2: Add Focus Styles to Theme.qml

**File: `qml/Theme.qml`**

Add focus-related properties:

```qml
pragma Singleton
import QtQuick

QtObject {
    // ... existing properties ...

    // Focus indicator styles
    readonly property color focusColor: primaryColor
    readonly property int focusBorderWidth: 3
    readonly property int focusMargin: 2
}
```

---

## Step 3: Update ActionButton with Focus Support

**File: `qml/components/ActionButton.qml`**

```qml
Button {
    id: control

    property string iconSource: ""
    property color backgroundColor: Theme.primaryColor

    // Enable keyboard focus
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true

    // Accessibility (from Phase 2)
    Accessible.role: Accessible.Button
    Accessible.name: control.text
    Accessible.focusable: true

    // ... existing implicitWidth, implicitHeight, contentItem ...

    background: Rectangle {
        radius: Theme.buttonRadius
        color: {
            if (!control.enabled) return Theme.buttonDisabled
            if (control._isPressed) return Qt.darker(control.backgroundColor, 1.2)
            if (control.hovered || control.activeFocus) return Qt.lighter(control.backgroundColor, 1.1)
            return control.backgroundColor
        }

        // Focus indicator
        Rectangle {
            anchors.fill: parent
            anchors.margins: -Theme.focusMargin
            visible: control.activeFocus
            color: "transparent"
            border.width: Theme.focusBorderWidth
            border.color: Theme.focusColor
            radius: parent.radius + Theme.focusMargin
        }

        Behavior on color {
            ColorAnimation { duration: 100 }
        }
    }

    // Keyboard handling
    Keys.onReturnPressed: {
        control._longPressTriggered = false
        control.clicked()
    }

    Keys.onEnterPressed: {
        control._longPressTriggered = false
        control.clicked()
    }

    Keys.onSpacePressed: {
        control._longPressTriggered = false
        control.clicked()
    }

    // Shift+Enter for long-press action
    Keys.onPressed: function(event) {
        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) &&
            (event.modifiers & Qt.ShiftModifier)) {
            control.pressAndHold()
            event.accepted = true
        }
    }

    // ... existing MouseArea code ...
}
```

---

## Step 4: Update ValueInput with Keyboard Support

**File: `qml/components/ValueInput.qml`**

```qml
Item {
    id: root

    // ... existing properties ...

    // Enable keyboard focus
    activeFocusOnTab: true
    focus: true

    // Accessibility
    Accessible.role: Accessible.Slider
    Accessible.name: root.displayText || (root.value.toFixed(root.decimals) + root.suffix)
    Accessible.focusable: true

    // ... existing implicitWidth, implicitHeight ...

    // Keyboard handling
    Keys.onUpPressed: adjustValue(1)
    Keys.onDownPressed: adjustValue(-1)
    Keys.onLeftPressed: adjustValue(-1)
    Keys.onRightPressed: adjustValue(1)

    Keys.onReturnPressed: scrubberPopup.open()
    Keys.onEnterPressed: scrubberPopup.open()
    Keys.onSpacePressed: scrubberPopup.open()

    // Page up/down for larger steps
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_PageUp) {
            adjustValue(10)
            event.accepted = true
        } else if (event.key === Qt.Key_PageDown) {
            adjustValue(-10)
            event.accepted = true
        }
    }

    // Focus indicator around the entire control
    Rectangle {
        anchors.fill: valueDisplay
        anchors.margins: -Theme.focusMargin
        visible: root.activeFocus
        color: "transparent"
        border.width: Theme.focusBorderWidth
        border.color: Theme.focusColor
        radius: valueDisplay.radius + Theme.focusMargin
        z: -1
    }

    // ... rest of existing code ...

    // Update popup to handle keyboard
    Popup {
        id: scrubberPopup
        // ... existing code ...

        // Focus management
        onOpened: {
            popupValueContainer.forceActiveFocus()
        }

        // Escape to close
        Keys.onEscapePressed: scrubberPopup.close()

        // Arrow keys for adjustment
        Keys.onUpPressed: adjustValue(1)
        Keys.onDownPressed: adjustValue(-1)
        Keys.onLeftPressed: adjustValue(-1)
        Keys.onRightPressed: adjustValue(1)

        // Enter to confirm and close
        Keys.onReturnPressed: scrubberPopup.close()
        Keys.onEnterPressed: scrubberPopup.close()
    }
}
```

---

## Step 5: Update IdlePage with KeyNavigation

**File: `qml/pages/IdlePage.qml`**

Add KeyNavigation between the four main buttons:

```qml
Page {
    id: idlePage

    // Set initial focus when page loads
    Component.onCompleted: espressoButton.forceActiveFocus()
    StackView.onActivated: espressoButton.forceActiveFocus()

    // ... existing code ...

    // In the grid/layout containing action buttons:
    ActionButton {
        id: espressoButton
        text: "Espresso"
        // ... existing properties ...

        KeyNavigation.right: steamButton
        KeyNavigation.down: hotWaterButton
    }

    ActionButton {
        id: steamButton
        text: "Steam"
        // ... existing properties ...

        KeyNavigation.left: espressoButton
        KeyNavigation.down: flushButton
    }

    ActionButton {
        id: hotWaterButton
        text: "Hot Water"
        // ... existing properties ...

        KeyNavigation.up: espressoButton
        KeyNavigation.right: flushButton
    }

    ActionButton {
        id: flushButton
        text: "Flush"
        // ... existing properties ...

        KeyNavigation.up: steamButton
        KeyNavigation.left: hotWaterButton
    }

    // Sleep and Settings buttons
    Button {
        id: sleepButton
        // ...

        KeyNavigation.up: hotWaterButton
        KeyNavigation.right: settingsButton
        activeFocusOnTab: true
    }

    Button {
        id: settingsButton
        // ...

        KeyNavigation.up: flushButton
        KeyNavigation.left: sleepButton
        activeFocusOnTab: true
    }
}
```

---

## Step 6: Update SteamPage (and similar) with KeyNavigation

**File: `qml/pages/SteamPage.qml`**

```qml
Page {
    id: steamPage

    Component.onCompleted: {
        // Focus first preset pill or first control
        if (presetRow.count > 0) {
            presetRow.itemAt(0).forceActiveFocus()
        }
    }

    // ... existing code ...

    // Preset pills - navigate with left/right
    // This requires updating PresetPillRow to support keyboard nav
    // See Step 7

    // Value controls - navigate with up/down
    ValueInput {
        id: durationInput
        // ...
        KeyNavigation.down: flowInput
    }

    ValueInput {
        id: flowInput
        // ...
        KeyNavigation.up: durationInput
        KeyNavigation.down: tempInput
    }

    ValueInput {
        id: tempInput
        // ...
        KeyNavigation.up: flowInput
    }
}
```

---

## Step 7: Update PresetPillRow with Keyboard Support

**File: `qml/components/PresetPillRow.qml`**

The preset row needs internal keyboard navigation:

```qml
Item {
    id: root

    // ... existing properties ...

    // Track focused pill index
    property int focusedIndex: -1

    // Keyboard navigation at row level
    Keys.onLeftPressed: {
        if (focusedIndex > 0) {
            focusedIndex--
            // Focus the pill at new index
        }
    }

    Keys.onRightPressed: {
        if (focusedIndex < model.count - 1) {
            focusedIndex++
            // Focus the pill at new index
        }
    }

    // ... existing Flow/Repeater code ...

    // In the pill delegate:
    Rectangle {
        id: pill
        // ... existing code ...

        activeFocusOnTab: index === 0  // Only first pill in tab order
        focus: root.focusedIndex === index

        // Focus indicator
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            visible: pill.activeFocus
            color: "transparent"
            border.width: 3
            border.color: Theme.focusColor
            radius: pill.radius + 2
        }

        Keys.onReturnPressed: {
            // Select this preset
            root.presetSelected(modelData)
        }

        Keys.onEnterPressed: {
            root.presetSelected(modelData)
        }

        // Double-tap Enter for edit (within 300ms)
        property var lastEnterTime: 0
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                var now = Date.now()
                if (now - lastEnterTime < 300) {
                    root.presetEditRequested(modelData)
                }
                lastEnterTime = now
            }
        }

        onActiveFocusChanged: {
            if (activeFocus) {
                root.focusedIndex = index
            }
        }
    }
}
```

---

## Step 8: Update SettingsPage with Tab Navigation

**File: `qml/pages/SettingsPage.qml`**

```qml
Page {
    // ... existing code ...

    TabBar {
        id: tabBar
        // ... existing code ...

        // Enable keyboard navigation between tabs
        Keys.onLeftPressed: {
            if (currentIndex > 0) currentIndex--
        }
        Keys.onRightPressed: {
            if (currentIndex < count - 1) currentIndex++
        }
    }

    // Each TabButton
    TabButton {
        // ... existing code ...
        activeFocusOnTab: true

        Keys.onReturnPressed: {
            tabBar.currentIndex = TabBar.index
        }
    }

    // Switches and inputs in tab content
    Switch {
        // ... existing code ...
        activeFocusOnTab: true

        Keys.onReturnPressed: toggle()
        Keys.onSpacePressed: toggle()
    }
}
```

---

## Step 9: Update EspressoPage with Minimal Keyboard Support

**File: `qml/pages/EspressoPage.qml`**

The espresso page is mostly display, but needs back button keyboard support:

```qml
Page {
    id: espressoPage

    // Keyboard shortcut to stop and go back
    Keys.onEscapePressed: {
        DE1Device.stopOperation()
        root.goToIdle()
    }

    // Space also stops
    Keys.onSpacePressed: {
        DE1Device.stopOperation()
        root.goToIdle()
    }

    // ... existing code ...

    // Back button in info bar
    Item {
        id: backButton
        // ... existing code ...

        activeFocusOnTab: true

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            visible: parent.activeFocus
            color: "transparent"
            border.width: 3
            border.color: Theme.focusColor
            radius: 4
        }

        Keys.onReturnPressed: {
            DE1Device.stopOperation()
            root.goToIdle()
        }
    }
}
```

---

## Step 10: Update BottomBar with Focus Support

**File: `qml/components/BottomBar.qml`**

```qml
Item {
    id: root

    // ... existing properties ...

    // Back button
    Item {
        id: backButton
        // ... existing code ...

        activeFocusOnTab: true

        // Focus indicator
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            visible: backButton.activeFocus
            color: "transparent"
            border.width: 3
            border.color: Theme.focusColor
            radius: 4
        }

        Keys.onReturnPressed: root.backClicked()
        Keys.onEnterPressed: root.backClicked()
        Keys.onEscapePressed: root.backClicked()

        MouseArea {
            // ... existing code ...
        }
    }
}
```

---

## Step 11: Global Keyboard Shortcuts in main.qml

**File: `qml/main.qml`**

Add global keyboard shortcuts:

```qml
ApplicationWindow {
    id: root

    // ... existing code ...

    // Global keyboard shortcuts
    Shortcut {
        sequence: "Escape"
        onActivated: {
            // Go back if not on idle page
            if (stackView.depth > 1) {
                stackView.pop()
            }
        }
    }

    // Quick access shortcuts (optional)
    Shortcut {
        sequence: "Ctrl+1"
        onActivated: {
            // Go to Espresso
            // ... navigation logic
        }
    }

    // ... more shortcuts as needed
}
```

---

## Step 12: Handle Dialog Focus

**File: `qml/main.qml`**

When dialogs open, move focus into them:

```qml
Dialog {
    id: bleErrorDialog
    // ... existing code ...

    onOpened: {
        // Focus the OK button
        okButton.forceActiveFocus()

        // Announce for accessibility
        if (AccessibilityManager.enabled) {
            AccessibilityManager.announce(title + ". " + contentText, true)
        }
    }

    Button {
        id: okButton
        text: "OK"
        activeFocusOnTab: true

        Keys.onReturnPressed: bleErrorDialog.close()
        Keys.onEscapePressed: bleErrorDialog.close()
    }
}
```

---

## Verification Checklist

After implementing Phase 3:

- [ ] Tab key moves focus between elements in logical order
- [ ] Arrow keys navigate within groups (buttons, presets)
- [ ] Enter/Space activates focused buttons
- [ ] Shift+Enter triggers long-press action
- [ ] Escape goes back or closes dialogs
- [ ] Focus indicator is clearly visible
- [ ] Dialogs trap focus when open
- [ ] Focus is restored when dialogs close
- [ ] All interactive elements are reachable by keyboard

---

## Files Modified/Created

| File | Action |
|------|--------|
| qml/components/FocusIndicator.qml | CREATE |
| qml/Theme.qml | MODIFY - add focus styles |
| qml/components/ActionButton.qml | MODIFY - add keyboard support |
| qml/components/ValueInput.qml | MODIFY - add keyboard support |
| qml/components/PresetPillRow.qml | MODIFY - add keyboard support |
| qml/components/BottomBar.qml | MODIFY - add keyboard support |
| qml/pages/IdlePage.qml | MODIFY - add KeyNavigation |
| qml/pages/SteamPage.qml | MODIFY - add KeyNavigation |
| qml/pages/HotWaterPage.qml | MODIFY - add KeyNavigation |
| qml/pages/FlushPage.qml | MODIFY - add KeyNavigation |
| qml/pages/SettingsPage.qml | MODIFY - add tab keyboard nav |
| qml/pages/EspressoPage.qml | MODIFY - add keyboard shortcuts |
| qml/main.qml | MODIFY - add global shortcuts, dialog focus |

---

## Notes for Implementation

1. **Focus order**: The natural tab order should follow visual layout (left-to-right, top-to-bottom). Use `activeFocusOnTab: true` to include elements in tab order.

2. **KeyNavigation**: Use `KeyNavigation.up/down/left/right` for 2D grid navigation (like the 4 action buttons on idle page).

3. **Focus trapping in dialogs**: When a dialog is open, Tab should cycle within the dialog only. QML Popups with `modal: true` should handle this automatically.

4. **Platform differences**: On Android with TalkBack, focus navigation uses swipe gestures. On desktop, Tab key is primary. Both work with the same `activeFocusOnTab` and keyboard handlers.

5. **Testing**: On desktop (Windows), you can test keyboard navigation directly. On Android, use TalkBack's "linear navigation" mode.
