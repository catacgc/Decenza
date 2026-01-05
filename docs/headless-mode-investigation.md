# Headless Mode Investigation

## Problem
Users with GHC (Group Head Controller) removed cannot start operations (espresso, steam, hot water, flush) from the app by tapping preset pills.

## Symptoms
- Pill tap is registered correctly
- Profile uploads successfully
- State change command (0x04 for Espresso) is sent to characteristic 0x0000A002
- Machine ACKs the write
- **But machine stays in Idle state (responds with 0x02 on 0x0000A00E)**

## Key Discovery
The machine reports GHC status as **4 ("debug")** instead of **0 ("not installed")** when GHC is physically removed.

```
DE1Device: GHC_INFO raw byte: 4 "(0x04)"
DE1Device: "GHC status: debug → app CAN start operations"
```

According to de1app's `ghc_required()` function, status 4 should allow app control - but the machine still refuses.

**Critical finding:** The original de1app also cannot start operations on this machine. This confirms it's a machine configuration issue, not a bug in our code.

## GHC Status Values (from de1app)
| Value | Meaning | App Can Start? |
|-------|---------|----------------|
| 0 | Not installed | Yes |
| 1 | Present but unused | Yes |
| 2 | Installed but inactive | Yes |
| 3 | Active | **No** (must use buttons) |
| 4 | Debug mode | Yes |

## BLE Communication Flow (working correctly)
```
1. User taps pill → [IdlePage] logs "Starting espresso..."
2. App writes GHC_MODE=1 to 0x803820 (MMR)
3. App writes 0x04 to 0x0000A002 (REQUESTED_STATE)
4. Machine ACKs write
5. Machine responds on 0x0000A00E with state 0x02 (Idle) ← PROBLEM
```

## Code Changes Made

### 1. Debug Logging (de1device.cpp)
- Logs all incoming BLE data with characteristic UUID
- Logs all outgoing writes
- Logs raw GHC byte before interpretation
- Logs MMR responses with address

### 2. GHC_MODE Write (de1device.cpp)
Before starting any operation, we now write `GHC_MODE = 1` to tell the machine the app wants control:
```cpp
writeMMR(DE1::MMR::GHC_MODE, 1);  // Address 0x803820
```

### 3. Idle-Before-Operation Pattern (de1device.cpp)
Like de1app, we send Idle state first if machine isn't already Idle:
```cpp
if (m_state != DE1::State::Idle) {
    requestState(DE1::State::Idle);
}
requestState(DE1::State::Espresso);
```

### 4. Pill Handler Logging (IdlePage.qml)
All 4 pill handlers (espresso, steam, hot water, flush) now log:
- Index and wasAlreadySelected
- MachineState.isReady value
- When operation starts or why it doesn't

## Files Modified
- `src/ble/de1device.cpp` - GHC_MODE write, debug logging, Idle-first pattern
- `src/ble/de1device.h` - Added `requestGHCStatus()` declaration
- `qml/pages/IdlePage.qml` - Debug logging in pill handlers

## Questions for Decent
1. Why does GHC report status 4 ("debug") when physically removed? Should be 0.
2. How to properly enable app control when GHC is removed?
3. Is there a machine setting or different MMR value needed?
4. What's the correct sequence to start operations in headless mode?

## Next Steps
1. Get explanation from Decent about GHC debug mode
2. May need to write different value to GHC_MODE or another MMR address
3. Machine might need factory reset or firmware update

## Log Snippets for Reference

### Successful command send (but machine refuses):
```
DE1Device: Writing to "0000a002" data: "04"
DE1Device: Write confirmed to "0000a002" data: "04"
DE1Device: Received from "0000a00e" data: "0200"  ← Stays Idle!
```

### GHC status read:
```
DE1Device: Received from "0000a005" data: "0480381c04000000..."
DE1Device: MMR response - address: "0x80381c" raw data: "..."
DE1Device: GHC_INFO raw byte: 4 "(0x04)"
```

## Related Code References
- `src/ble/protocol/de1characteristics.h` - State enum, MMR addresses
- de1app `vars.tcl` - `ghc_required()` function
- de1app `machine.tcl` - `start_espresso` procedure
