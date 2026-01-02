# Steam Heater Idle Setting - Implementation Notes

## Problem
The steam heater appears to turn off when the DE1 machine is idle, causing delays when making milk drinks (need to wait for heating).

## What We Implemented

### Setting Added (commit c7b843e)
- `Settings.keepSteamHeaterOn` (bool, defaults to false)
- Stored as `steam/keepHeaterOn` in QSettings

### Files Modified
- `src/core/settings.h` - Added Q_PROPERTY, getter/setter declarations
- `src/core/settings.cpp` - Added getter/setter implementation
- `qml/pages/settings/SettingsPreferencesTab.qml` - Added toggle UI under Flow Sensor Calibration
- `qml/main.qml` - Added logic to resend steam settings when machine goes to Idle/Ready

### How It Works
When `keepSteamHeaterOn` is true:
1. When machine phase changes to Idle or Ready, `MainController.applySteamSettings()` is called
2. This resends the configured steam temperature to the DE1
3. The idea is to counteract any internal DE1 timeout that might reduce heater power

## Further Improvements (January 2026)

### Issue
The DE1 machine appears to have an internal timeout that reduces steam heater power after extended idle time. The previous implementation only sent settings on phase transitions, which wasn't enough.

### Changes Made
1. **Added periodic timer** (`qml/main.qml` line ~184):
   - `steamHeaterTimer` runs every 60 seconds
   - Only active when `keepSteamHeaterOn` is true AND machine is in Idle/Ready phase AND connected
   - Resends steam settings periodically to maintain target temperature

2. **Reduced initial settings delay** (`src/controllers/maincontroller.cpp` line 36):
   - Changed from 5 seconds to 1 second after connection
   - Ensures user's steam temperature settings are applied quickly at startup

### Root Cause Analysis
Comparing with de1app (reference implementation):
- de1app sends `de1_send_steam_hotwater_settings` at connection (`bluetooth.tcl:2072`)
- de1app includes TargetSteamTemp in ShotSettings characteristic
- The DE1 machine may internally reduce heater power after extended idle time
- No explicit "keep heater on" BLE command exists - must resend temperature periodically

### Key Code Locations
- Steam settings applied: `src/controllers/maincontroller.cpp` line 809 (`applySteamSettings()`)
- Shot settings sent to DE1: `src/ble/de1device.cpp` line 850 (`setShotSettings()`)
- Phase change handler: `qml/main.qml` line ~1050 (`onPhaseChanged`)
- Periodic timer: `qml/main.qml` line ~184 (`steamHeaterTimer`)
