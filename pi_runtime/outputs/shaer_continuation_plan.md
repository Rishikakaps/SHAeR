# SHAeR Continuation Plan

Date: 2026-07-05

## Where This Resumes

The previous SHAeR work created a C++ foreground runtime, HAL boundaries, simulator, web theme-world preview, settings/power/theme cleanup, and validation tests. The current repo is here:

`/Users/rishika/Documents/Codex/2026-06-26/thi/work/shaer`

The current active repo layer is:

Layer 3: Settings Application And OS Services

The repo already passes:

```bash
make check
```

I added software validation for Settings-driven shutdown and restart confirmations, so the current Settings layer now protects one of the most important physical-device flows before Raspberry Pi deployment.

## Big Picture

SHAeR should be built in layers, not feature piles:

1. Hardware substrate: display, controls, DAC, battery, RTC, power board, safe shutdown.
2. Core runtime: boot, logging, settings, service scheduler, recovery, power state.
3. UI shell: navigation, settings, status surfaces, display renderer.
4. Local-first music: SD scan, SQLite library, local playback, volume, gapless, crossfade, ReplayGain.
5. Connectivity: Bluetooth and Wi-Fi behavior.
6. Spotify: only after the device works fully offline.
7. Personal layers: voice archive, notifications, companion app, hidden features.
8. Physical finish: breadboard, perfboard, enclosure, thermal/battery profiling.

## What Changed In This Pass

File changed:

`/Users/rishika/Documents/Codex/2026-06-26/thi/work/shaer/tests/settings_services_tests.cpp`

Added test coverage for:

- Settings action contract.
- Settings -> Power -> Shutdown confirmation popup.
- Confirmed shutdown stopping runtime without reboot.
- Settings -> Power -> Restart confirmation popup.
- Confirmed restart stopping runtime with `reboot_requested`.

Validation:

```bash
make check
```

Result: PASS.

## Important Hardware Note

The final architecture Doc 21 has a wiring conflict:

- It says GPIO0/GPIO1 are reserved.
- It says GPIO2/GPIO3 are I2C.
- Later, it assigns LEFT to GPIO1 and RIGHT to GPIO2.

That should not be soldered as written. For now, treat the repo file below as the firmware-facing source of truth:

`/Users/rishika/Documents/Codex/2026-06-26/thi/work/shaer/docs/GPIO_MASTER_TABLE.md`

## Next Layer Step

Do not jump to Spotify or final theme polish yet. The next real step is Raspberry Pi breadboard validation for Settings and OS services:

```bash
cd /opt/shaer
make pi
sudo make install
sudo systemctl restart shaer
make settings_test
make power_test
make brightness_test
make storage_test
make battery_test
make wifi_test
make shutdown_test
make reboot_test
make developer_mode_test
```

Layer 3 should be tagged only after settings navigation, persistence across reboot, sleep, shutdown, reboot, wake, and diagnostics pass on the physical Pi.
