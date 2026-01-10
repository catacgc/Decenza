import QtQuick
import QtQuick.Controls
import DecenzaDE1

// Container that shifts content up when keyboard appears
// Usage:
//   KeyboardAwareContainer {
//       textFields: [myTextField1, myTextField2]
//       YourContent { ... }
//   }

Item {
    id: root

    // List of text fields to track for keyboard offset
    property var textFields: []

    // Current keyboard offset
    property real keyboardOffset: 0

    // Content is the default property so children go inside automatically
    default property alias content: contentContainer.data

    // Delay timer to prevent jumpy behavior when switching fields
    Timer {
        id: keyboardResetTimer
        interval: 300
        onTriggered: {
            if (!Qt.inputMethod.visible && !hasActiveFocus()) {
                root.keyboardOffset = 0
            }
        }
    }

    // Check if any registered text field has focus
    function hasActiveFocus() {
        for (var i = 0; i < textFields.length; i++) {
            if (textFields[i] && textFields[i].activeFocus) {
                return true
            }
        }
        return false
    }

    // Get the currently focused text field
    function getActiveFocusField() {
        for (var i = 0; i < textFields.length; i++) {
            if (textFields[i] && textFields[i].activeFocus) {
                return textFields[i]
            }
        }
        return null
    }

    // Calculate keyboard offset for a given field
    function updateKeyboardOffset(focusedField) {
        if (!focusedField) return

        // Calculate based on field position - shift to top 30% of screen
        // Add current offset back to get ORIGINAL position (before any shift)
        var fieldPos = focusedField.mapToItem(root, 0, 0)
        var fieldBottomOriginal = fieldPos.y + focusedField.height + keyboardOffset
        var safeZone = root.height * 0.30
        var newOffset = Math.max(0, fieldBottomOriginal - safeZone)

        keyboardOffset = newOffset
    }

    // Track keyboard visibility
    Connections {
        target: Qt.inputMethod
        function onVisibleChanged() {
            if (Qt.inputMethod.visible) {
                keyboardResetTimer.stop()
                var focusedField = root.getActiveFocusField()
                if (focusedField) {
                    root.updateKeyboardOffset(focusedField)
                }
            } else {
                // Keyboard hidden - but if field still has focus, try to re-show it
                if (root.hasActiveFocus()) {
                    Qt.inputMethod.show()
                } else {
                    keyboardResetTimer.restart()
                }
            }
        }
    }

    // Tap outside to dismiss keyboard
    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: {
            var focusedField = root.getActiveFocusField()
            if (focusedField) {
                focusedField.focus = false
                Qt.inputMethod.hide()
            }
        }
    }

    // Content container that shifts with keyboard
    Item {
        id: contentContainer
        width: parent.width
        height: parent.height
        y: -root.keyboardOffset

        Behavior on y {
            NumberAnimation { duration: 250; easing.type: Easing.OutQuad }
        }
    }

    // Helper function to register a text field for keyboard handling
    // Call this from Component.onCompleted of your text fields
    function registerTextField(field) {
        if (field && textFields.indexOf(field) === -1) {
            textFields.push(field)

            // Connect to activeFocusChanged
            field.activeFocusChanged.connect(function() {
                if (field.activeFocus && Qt.inputMethod.visible) {
                    root.updateKeyboardOffset(field)
                }
            })
        }
    }
}
