# SHAeR Integration Map

## Authority By Subsystem

| Subsystem | Source of truth now | Integration decision |
| --- | --- | --- |
| Foreground lifecycle | `native_firmware/firmware/core` | Keep native C++ as the sole foreground runtime. |
| Navigation/state | Native C++ plus July JS contract | Port missing July invariants/tests into C++; do not keep two production state machines. |
| Hardware drivers | `native_firmware/firmware/hal` | Keep native HAL; use `pi_runtime/.../shaer_hal` as service/host test adapters only. |
| Themes/layout | `pi_runtime/outputs/shaer_*` | Treat as the visual oracle and asset source for the native renderer. |
| Theme regression | `pi_runtime/outputs/theme_validation` | Keep as the screenshot/layout gate during native rendering migration. |
| Local library | Both tracks | Converge onto one SQLite schema and one provider API before migration. |
| Spotify Connect | `pi_runtime/outputs/shaer_backend/shaer_music` | Run as a supervised service behind source-neutral IPC. |
| Voice archive | `pi_runtime/outputs/shaer_backend/shaer_recording` | Keep service implementation; expose status/control/events to native UI. |
| Companion app | `pi_runtime/outputs/shaer_companion` | Keep protocol/security implementation; remove duplicate old companion surfaces after parity review. |
| Settings | Both tracks | Native settings catalog owns UI contract; persisted values must map to real service actions. |
| Power/shutdown | Native lifecycle plus July capability security | Preserve physical one-use confirmation and native safe-shutdown sequencing. |
| Deployment | Both tracks | Produce one systemd graph and one release package after IPC convergence. |

## Do Not Blindly Merge These

- app-state and Back/history implementations
- SQLite database files or schemas
- systemd units
- settings catalogs
- local music queue/playback state
- GPIO pin definitions
- companion authentication/pairing code

For each overlap, first write a contract test, choose one owner, migrate callers, and only then remove the superseded implementation.

## Desired Runtime Shape

```text
Native C++ foreground UI
        |
        +-- semantic input and native HAL
        +-- canonical UI state machine
        +-- native theme renderer
        |
        +-- IPC provider boundary
              +-- local library/indexer
              +-- Spotify/Connect
              +-- recording archive
              +-- companion/device management
```

Only the native foreground process should own pages, focus, Back, overlays, animations, and currently selected media. Services publish capabilities and state; they do not navigate the UI.
