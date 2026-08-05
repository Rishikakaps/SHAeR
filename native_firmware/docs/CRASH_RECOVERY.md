# SHAeR Crash Recovery V1

Status: Required before hardware testing.

## Goal

SHAeR must remain recoverable if firmware crashes, a theme is broken, settings are corrupt, or an update fails.

## Boot State Keys

| Key | Meaning |
|---|---|
| `boot.counter` | Total attempted firmware boots |
| `boot.in_progress` | `1` while boot/runtime has not reached the success checkpoint |
| `boot.last_start_epoch` | Unix time for current boot start |
| `boot.last_successful_epoch` | Unix time for last successful SHAeR OS start |
| `boot.first_crash_epoch` | First crash in current crash window |
| `boot.crash_counter` | Number of crash-loop starts |
| `boot.safe_mode` | `1` when Safe Mode is active |

## Crash Loop Rule

```text
3 incomplete boots within 30 seconds
-> Safe Mode
-> Archive Dark minimal theme
-> normal power profile
-> diagnostics available
-> firmware update still allowed
```

## Safe Mode Behavior

| Subsystem | Safe Mode Behavior |
|---|---|
| Theme | Force `archive_dark` |
| Animation | Minimal motion |
| Audio | Local test only until user acts |
| Spotify | Do not auto-start |
| Bluetooth | Do not block Home |
| Wi-Fi | Testable from diagnostics |
| Companion/Firmware update | Must remain available |

## Success Checkpoint

Boot is marked successful only after the firmware runtime accepts the boot report and reaches SHAeR OS. A display-only boot animation is not enough.

