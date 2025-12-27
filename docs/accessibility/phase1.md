# Phase 1: Accessibility Foundation

## Goal
Create the core accessibility infrastructure: AccessibilityManager C++ class, settings persistence, and a backdoor gesture for blind users to enable accessibility without navigating settings.

## Prerequisites
- Qt 6.8+ with Qt TextToSpeech and Qt Multimedia modules
- Understanding of existing Settings class pattern

---

## Step 1: Add Qt Modules to CMakeLists.txt

**File: `CMakeLists.txt`**

Add these modules to find_package:
```cmake
find_package(Qt6 REQUIRED COMPONENTS ... TextToSpeech Multimedia)
```

Add to target_link_libraries:
```cmake
target_link_libraries(decenza_de1 PRIVATE ... Qt6::TextToSpeech Qt6::Multimedia)
```

---

## Step 2: Create AccessibilityManager Header

**File: `src/core/accessibilitymanager.h`** (NEW FILE)

```cpp
#ifndef ACCESSIBILITYMANAGER_H
#define ACCESSIBILITYMANAGER_H

#include <QObject>
#include <QTextToSpeech>
#include <QSoundEffect>
#include <QSettings>

class AccessibilityManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool ttsEnabled READ ttsEnabled WRITE setTtsEnabled NOTIFY ttsEnabledChanged)
    Q_PROPERTY(bool tickEnabled READ tickEnabled WRITE setTickEnabled NOTIFY tickEnabledChanged)
    Q_PROPERTY(int verbosity READ verbosity WRITE setVerbosity NOTIFY verbosityChanged)

public:
    explicit AccessibilityManager(QObject *parent = nullptr);
    ~AccessibilityManager();

    // Verbosity levels
    enum Verbosity {
        Minimal = 0,   // Start/stop + errors only
        Normal = 1,    // + milestones (pressure reached, weight reached)
        Verbose = 2    // + periodic status updates
    };
    Q_ENUM(Verbosity)

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool ttsEnabled() const { return m_ttsEnabled; }
    void setTtsEnabled(bool enabled);

    bool tickEnabled() const { return m_tickEnabled; }
    void setTickEnabled(bool enabled);

    int verbosity() const { return m_verbosity; }
    void setVerbosity(int level);

    // Called from QML
    Q_INVOKABLE void announce(const QString& text, bool interrupt = false);
    Q_INVOKABLE void playTick();
    Q_INVOKABLE void toggleEnabled();  // For backdoor gesture

signals:
    void enabledChanged();
    void ttsEnabledChanged();
    void tickEnabledChanged();
    void verbosityChanged();

private:
    void loadSettings();
    void saveSettings();
    void initTts();
    void initTickSound();

    bool m_enabled = false;
    bool m_ttsEnabled = true;
    bool m_tickEnabled = true;
    int m_verbosity = Normal;

    QTextToSpeech* m_tts = nullptr;
    QSoundEffect* m_tickSound = nullptr;
    QSettings m_settings;
};

#endif // ACCESSIBILITYMANAGER_H
```

---

## Step 3: Create AccessibilityManager Implementation

**File: `src/core/accessibilitymanager.cpp`** (NEW FILE)

```cpp
#include "accessibilitymanager.h"
#include <QDebug>
#include <QCoreApplication>

AccessibilityManager::AccessibilityManager(QObject *parent)
    : QObject(parent)
    , m_settings("Decenza", "DE1")
{
    loadSettings();
    initTts();
    initTickSound();
}

AccessibilityManager::~AccessibilityManager()
{
    if (m_tts) {
        m_tts->stop();
    }
}

void AccessibilityManager::loadSettings()
{
    m_enabled = m_settings.value("accessibility/enabled", false).toBool();
    m_ttsEnabled = m_settings.value("accessibility/ttsEnabled", true).toBool();
    m_tickEnabled = m_settings.value("accessibility/tickEnabled", true).toBool();
    m_verbosity = m_settings.value("accessibility/verbosity", Normal).toInt();
}

void AccessibilityManager::saveSettings()
{
    m_settings.setValue("accessibility/enabled", m_enabled);
    m_settings.setValue("accessibility/ttsEnabled", m_ttsEnabled);
    m_settings.setValue("accessibility/tickEnabled", m_tickEnabled);
    m_settings.setValue("accessibility/verbosity", m_verbosity);
    m_settings.sync();
}

void AccessibilityManager::initTts()
{
    m_tts = new QTextToSpeech(this);

    // Use default engine (platform-specific)
    // Android: uses Android TTS
    // iOS: uses AVSpeechSynthesizer
    // Desktop: uses platform TTS (SAPI on Windows, etc.)

    connect(m_tts, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
        if (state == QTextToSpeech::Error) {
            qWarning() << "TTS error:" << m_tts->errorString();
        }
    });
}

void AccessibilityManager::initTickSound()
{
    m_tickSound = new QSoundEffect(this);
    m_tickSound->setSource(QUrl("qrc:/sounds/tick.wav"));
    m_tickSound->setVolume(0.5);
}

void AccessibilityManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    saveSettings();
    emit enabledChanged();

    // Announce state change
    if (m_enabled && m_ttsEnabled) {
        announce("Accessibility enabled", true);
    }
}

void AccessibilityManager::setTtsEnabled(bool enabled)
{
    if (m_ttsEnabled == enabled) return;
    m_ttsEnabled = enabled;
    saveSettings();
    emit ttsEnabledChanged();
}

void AccessibilityManager::setTickEnabled(bool enabled)
{
    if (m_tickEnabled == enabled) return;
    m_tickEnabled = enabled;
    saveSettings();
    emit tickEnabledChanged();
}

void AccessibilityManager::setVerbosity(int level)
{
    if (m_verbosity == level) return;
    m_verbosity = qBound(0, level, 2);
    saveSettings();
    emit verbosityChanged();
}

void AccessibilityManager::announce(const QString& text, bool interrupt)
{
    if (!m_enabled || !m_ttsEnabled || !m_tts) return;

    if (interrupt) {
        m_tts->stop();
    }

    m_tts->say(text);
    qDebug() << "Accessibility announcement:" << text;
}

void AccessibilityManager::playTick()
{
    if (!m_enabled || !m_tickEnabled || !m_tickSound) return;

    m_tickSound->play();
}

void AccessibilityManager::toggleEnabled()
{
    setEnabled(!m_enabled);

    // Always announce toggle result, even if TTS was just disabled
    if (m_tts && m_ttsEnabled) {
        m_tts->stop();
        m_tts->say(m_enabled ? "Accessibility enabled" : "Accessibility disabled");
    }
}
```

---

## Step 4: Create Tick Sound Resource

**File: `resources/sounds/tick.wav`** (NEW FILE)

Create a short tick sound (~50ms, 440Hz sine wave with quick decay). This can be generated with Audacity or any audio tool.

Alternatively, use a system beep by modifying `playTick()` to use `QApplication::beep()` as fallback.

**Update: `resources.qrc`**

Add to the qrc file:
```xml
<qresource prefix="/">
    <file>sounds/tick.wav</file>
</qresource>
```

---

## Step 5: Register AccessibilityManager in main.cpp

**File: `src/main.cpp`**

Add include:
```cpp
#include "core/accessibilitymanager.h"
```

In main(), after creating the QML engine but before loading the main QML file:
```cpp
// Create accessibility manager
AccessibilityManager* accessibilityManager = new AccessibilityManager(&app);

// Expose to QML
engine.rootContext()->setContextProperty("AccessibilityManager", accessibilityManager);
```

---

## Step 6: Add Backdoor Gesture to main.qml

**File: `qml/main.qml`**

Add at the top level (inside the root ApplicationWindow or Window), similar to the simulation mode toggle in SettingsPage:

```qml
// Accessibility backdoor: 5-finger tap to toggle accessibility
property int accessibilityTapCount: 0

Timer {
    id: accessibilityTapResetTimer
    interval: 2000
    onTriggered: accessibilityTapCount = 0
}

// Accessibility activation toast
Rectangle {
    id: accessibilityToast
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: parent.top
    anchors.topMargin: Theme.scaled(100)
    width: accessibilityToastText.implicitWidth + 40
    height: accessibilityToastText.implicitHeight + 20
    radius: height / 2
    color: AccessibilityManager.enabled ? Theme.successColor : "#333"
    opacity: 0
    z: 9999

    Text {
        id: accessibilityToastText
        anchors.centerIn: parent
        text: AccessibilityManager.enabled ? "Accessibility ON" : "Accessibility OFF"
        color: "white"
        font.pixelSize: 18
        font.bold: true
    }

    Behavior on opacity { NumberAnimation { duration: 150 } }

    Timer {
        id: accessibilityToastHideTimer
        interval: 2000
        onTriggered: accessibilityToast.opacity = 0
    }
}

// 5-finger touch detection (works on tablets)
MultiPointTouchArea {
    anchors.fill: parent
    z: -1  // Behind all controls
    minimumTouchPoints: 5
    maximumTouchPoints: 5

    onPressed: function(touchPoints) {
        if (touchPoints.length === 5) {
            accessibilityTapCount++
            accessibilityTapResetTimer.restart()

            if (accessibilityTapCount >= 1) {  // Single 5-finger tap is enough
                AccessibilityManager.toggleEnabled()
                accessibilityToast.opacity = 1
                accessibilityToastHideTimer.restart()
                accessibilityTapCount = 0
            }
        }
    }
}

// Alternative: 3-finger triple-tap (more compatible with smaller screens)
// This can be added as an alternative activation method
MouseArea {
    anchors.fill: parent
    z: -2  // Behind the MultiPointTouchArea

    property int tripleClickCount: 0
    property var lastClickTime: 0

    Timer {
        id: tripleClickResetTimer
        interval: 1000
        onTriggered: parent.tripleClickCount = 0
    }

    // Note: This MouseArea is a fallback; primary method is 5-finger tap
}
```

---

## Step 7: Add Accessibility Settings to SettingsPage

**File: `qml/pages/SettingsPage.qml`**

Add a new TabButton for "Access" (short for Accessibility):

```qml
TabButton {
    text: "Access"
    width: implicitWidth
    font.pixelSize: 14
    font.bold: tabBar.currentIndex === 5  // Adjust index based on position
    contentItem: Text {
        text: parent.text
        font: parent.font
        color: tabBar.currentIndex === 5 ? Theme.primaryColor : Theme.textSecondaryColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        color: tabBar.currentIndex === 5 ? Theme.surfaceColor : "transparent"
        radius: 6
    }
}
```

Add the accessibility tab content in StackLayout:

```qml
// ============ ACCESSIBILITY TAB ============
Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Main card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 350
            color: Theme.surfaceColor
            radius: Theme.cardRadius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                Text {
                    text: "Accessibility"
                    color: Theme.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                Text {
                    text: "Enable screen reader support and audio feedback for blind and visually impaired users"
                    color: Theme.textSecondaryColor
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Tip about activation
                Rectangle {
                    Layout.fillWidth: true
                    height: tipText.implicitHeight + 16
                    radius: 6
                    color: Qt.rgba(Theme.primaryColor.r, Theme.primaryColor.g, Theme.primaryColor.b, 0.15)
                    border.color: Theme.primaryColor
                    border.width: 1

                    Text {
                        id: tipText
                        anchors.fill: parent
                        anchors.margins: 8
                        text: "Tip: 5-finger tap anywhere to toggle accessibility on/off"
                        color: Theme.primaryColor
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { height: 10 }

                // Enable toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15

                    Text {
                        text: "Enable Accessibility"
                        color: Theme.textColor
                        font.pixelSize: 14
                    }

                    Item { Layout.fillWidth: true }

                    Switch {
                        checked: AccessibilityManager.enabled
                        onCheckedChanged: AccessibilityManager.enabled = checked
                    }
                }

                // TTS toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    opacity: AccessibilityManager.enabled ? 1.0 : 0.5

                    Text {
                        text: "Voice Announcements"
                        color: Theme.textColor
                        font.pixelSize: 14
                    }

                    Item { Layout.fillWidth: true }

                    Switch {
                        checked: AccessibilityManager.ttsEnabled
                        enabled: AccessibilityManager.enabled
                        onCheckedChanged: AccessibilityManager.ttsEnabled = checked
                    }
                }

                // Tick sound toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    opacity: AccessibilityManager.enabled ? 1.0 : 0.5

                    ColumnLayout {
                        spacing: 2
                        Text {
                            text: "Frame Tick Sound"
                            color: Theme.textColor
                            font.pixelSize: 14
                        }
                        Text {
                            text: "Play a tick when extraction frames change"
                            color: Theme.textSecondaryColor
                            font.pixelSize: 12
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Switch {
                        checked: AccessibilityManager.tickEnabled
                        enabled: AccessibilityManager.enabled
                        onCheckedChanged: AccessibilityManager.tickEnabled = checked
                    }
                }

                // Verbosity selector
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    opacity: AccessibilityManager.enabled ? 1.0 : 0.5

                    Text {
                        text: "Announcement Verbosity"
                        color: Theme.textSecondaryColor
                        font.pixelSize: 12
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { value: 0, label: "Minimal", desc: "Start/stop only" },
                                { value: 1, label: "Normal", desc: "Milestones" },
                                { value: 2, label: "Verbose", desc: "Periodic updates" }
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 50
                                radius: 6
                                color: AccessibilityManager.verbosity === modelData.value ?
                                       Theme.primaryColor : Theme.backgroundColor
                                border.color: AccessibilityManager.verbosity === modelData.value ?
                                              Theme.primaryColor : Theme.textSecondaryColor
                                border.width: 1
                                opacity: AccessibilityManager.enabled ? 1.0 : 0.5

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 2

                                    Text {
                                        text: modelData.label
                                        color: AccessibilityManager.verbosity === modelData.value ?
                                               "white" : Theme.textColor
                                        font.pixelSize: 14
                                        font.bold: true
                                        Layout.alignment: Qt.AlignHCenter
                                    }

                                    Text {
                                        text: modelData.desc
                                        color: AccessibilityManager.verbosity === modelData.value ?
                                               Qt.rgba(1, 1, 1, 0.7) : Theme.textSecondaryColor
                                        font.pixelSize: 10
                                        Layout.alignment: Qt.AlignHCenter
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: AccessibilityManager.enabled
                                    onClicked: AccessibilityManager.verbosity = modelData.value
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Test button
                Button {
                    text: "Test Announcement"
                    enabled: AccessibilityManager.enabled && AccessibilityManager.ttsEnabled
                    onClicked: AccessibilityManager.announce("Accessibility is working correctly", true)
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
```

---

## Verification Checklist

After implementing Phase 1:

- [ ] App compiles with new C++ files
- [ ] AccessibilityManager is exposed to QML
- [ ] 5-finger tap shows toast and toggles accessibility
- [ ] Toast announces "Accessibility ON/OFF"
- [ ] Settings tab shows Accessibility options
- [ ] Toggling settings in UI updates AccessibilityManager properties
- [ ] Settings persist after app restart
- [ ] Test Announcement button speaks text

---

## Files Modified/Created

| File | Action |
|------|--------|
| CMakeLists.txt | MODIFY - add Qt modules |
| src/core/accessibilitymanager.h | CREATE |
| src/core/accessibilitymanager.cpp | CREATE |
| src/main.cpp | MODIFY - register AccessibilityManager |
| resources/sounds/tick.wav | CREATE |
| resources.qrc | MODIFY - add sound resource |
| qml/main.qml | MODIFY - add backdoor gesture |
| qml/pages/SettingsPage.qml | MODIFY - add Accessibility tab |

---

## Notes for Implementation

1. **Qt TextToSpeech availability**: On some platforms, TTS may not be available. Handle this gracefully by checking `m_tts->availableEngines()`.

2. **Tick sound generation**: If you don't have a tick.wav file, you can:
   - Use Audacity: Generate > Tone > 440Hz, 0.05 seconds, sine wave
   - Use online generators
   - Use `QApplication::beep()` as fallback

3. **5-finger detection on small screens**: If the tablet is small, 5 fingers may be difficult. The 3-finger triple-tap is an alternative but more complex to implement correctly.

4. **Settings persistence**: The AccessibilityManager uses the same QSettings organization as the rest of the app ("Decenza", "DE1").
