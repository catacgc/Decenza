# Smart Battery Charging System

This document explains how the DE1 smart battery charging feature works.

## Overview

The DE1 espresso machine has a USB port that powers/charges the tablet. The DE1 can control whether this USB port delivers power or not via a Memory-Mapped Register (MMR). The tablet monitors its own battery level and sends BLE commands to the DE1 to turn charging on/off, keeping the battery in an optimal range (55-65%) to extend battery lifespan.

## How It Works

```
┌─────────────┐                    ┌─────────────┐
│   Tablet    │                    │     DE1     │
│             │                    │             │
│ Read battery├───── BLE ─────────►│ MMR 0x803854│
│ level (JNI) │  "turn off USB"    │ USB Control │
│             │                    │             │
│ 55-65%      │◄──── USB Power ────┤ USB Port    │
│ target range│   (on or off)      │             │
└─────────────┘                    └─────────────┘
```

1. **Tablet reads its own battery** via Android system API (JNI)
2. **BatteryManager decides** if charging should be on/off based on battery level
3. **Tablet sends BLE command** to DE1's MMR address 0x803854
4. **DE1 controls USB power** - value 1 = ON, value 0 = OFF

## Charging Modes

| Mode | Setting | Behavior |
|------|---------|----------|
| Off | 0 | Charger always ON (no smart control) |
| On | 1 | Maintain 55-65% charge (default) |
| Night | 2 | Maintain 90-95% when active, 15-95% when sleeping |

## Smart Charging Logic (Mode "On")

Uses hysteresis to prevent rapid on/off cycling:

```
Battery Level:  0%----55%----65%----100%
                      │      │
                      ▼      ▼
              Turn ON here   Turn OFF here
```

- **Discharging cycle** (charger OFF): Wait until battery drops to 55%, then turn ON
- **Charging cycle** (charger ON): Wait until battery reaches 65%, then turn OFF
- State tracked via `m_discharging` flag

## DE1's 10-Minute Timeout

The DE1 has a safety feature that automatically turns the charger back ON after 10 minutes. This prevents the tablet from dying if the app crashes. To maintain charger OFF state, we must resend the command every 60 seconds (matching de1app behavior).

## Code Flow

```
Timer (60s) → BatteryManager::checkBattery()
                    │
                    ▼
            readPlatformBatteryPercent()  [Android JNI]
                    │
                    ▼
            applySmartCharging()
                    │
                    ▼
            DE1Device::setUsbChargerOn(on, force=true)
                    │
                    ▼
            DE1Device::writeMMR(0x803854, value)
                    │
                    ▼
            queueCommand() → writeCharacteristic()
                    │
                    ▼
            BLE write to characteristic A006 (WriteToMMR)
```

## BLE Command Format

MMR Write to characteristic `0000A006-0000-1000-8000-00805F9B34FB`:

```
Byte 0:     0x04              Length (4 bytes)
Bytes 1-3:  0x80 0x38 0x54    Address (big-endian)
Bytes 4-7:  0x01/0x00 + zeros Value (little-endian, 1=ON, 0=OFF)
Bytes 8-19: 0x00              Padding
```

This matches the de1app Tcl implementation exactly:
```tcl
mmr_write "set_usb_charger_on" "803854" "04" [zero_pad [int_to_hex $usbon] 2]
```

## Key Files

| File | Purpose |
|------|---------|
| `src/core/batterymanager.h` | BatteryManager class definition |
| `src/core/batterymanager.cpp` | Smart charging logic, timer, Android JNI battery reading |
| `src/ble/de1device.cpp` | `setUsbChargerOn()`, `writeMMR()` - BLE command sending |
| `src/ble/protocol/de1characteristics.h` | MMR address definitions, BLE UUIDs |
| `qml/pages/SettingsPage.qml` | UI for battery status and mode selection |

## Debug Logging

Debug statements are included throughout the code path:

```
BatteryManager: checkBattery - battery: X% mode: Y discharging: Z connected: true/false
BatteryManager: Sending charger command: ON/OFF
DE1Device::setUsbChargerOn - writing MMR: ON/OFF address: 0x803854
DE1Device::writeMMR - queuing command for address: 0x803854 value: 0/1 data: 0480385401000000
```

Check logs to verify:
1. Timer is firing every 60 seconds
2. Battery percentage is being read correctly
3. Correct charging decision is being made
4. MMR command is being queued and sent

## Testing Notes

**Important:** This feature only works when the tablet is powered by the DE1's USB port. When connected to a PC for development, the PC provides power and the DE1's charger control has no effect.

To test properly:
1. Disconnect tablet from PC
2. Connect tablet only to DE1's USB port
3. Set smart charging to "On" in Settings
4. Wait for battery to reach 65% - charger should turn OFF
5. Wait for battery to drop to 55% - charger should turn ON

## Reference: de1app Implementation

The original Tcl implementation in de1app:
- File: `de1plus/utils.tcl` - `check_battery_charger` proc (lines 536-590)
- File: `de1plus/de1_comms.tcl` - `set_usb_charger_on` proc (lines 1147-1160)
- Timer: `schedule_minute_task` runs every 60 seconds

## Investigation Notes (Dec 2024)

Issue reported: "Battery charge limits don't work"

Analysis performed:
1. Compared de1-qt implementation with de1app (Tcl) - logic is identical
2. Verified MMR command format matches exactly
3. Added debug logging throughout code path
4. Battery reading via Android JNI confirmed working (UI shows correct level)
5. Possible cause: Testing while connected to PC (PC provides power, not DE1)

Next steps if issue persists after DE1 testing:
- Check debug logs for command flow
- Verify BLE characteristic A006 is discovered
- Verify command queue is processing
- Consider adding read-back of MMR to confirm DE1 received command
