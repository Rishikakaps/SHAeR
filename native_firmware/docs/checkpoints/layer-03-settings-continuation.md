# Layer 3 Settings Continuation Checkpoint

Date: 2026-07-05

## Current Context

The repo is continuing from the SHAeR starter HAL and simulator work, but the newer architecture documents describe the final Raspberry Pi product as a layered Linux device: Raspberry Pi OS Lite, systemd services, hardware diagnostics, foreground SHAeR UI, local-first playback, and later Spotify, voice, companion, notifications, and hidden features.

The active repo checkpoint is still Layer 3: Settings Application And OS Services. Do not jump to Spotify, voice recording, companion app, or final theme polish until this layer has passed software and breadboard validation.

## Software Validation Added

The settings service test suite now covers:

- Required settings categories.
- Settings action classification.
- SQLite persistence across reload.
- Settings navigation and brightness update.
- Settings-driven shutdown confirmation.
- Settings-driven restart confirmation.
- Developer mode, sleep, shutdown, and reboot state transitions.

Validation command:

```bash
make check
```

Result on 2026-07-05: PASS.

## Big Picture Build Layers

1. Hardware substrate: Pi Zero 2 W, SPI TFT, EC11 controls, PCM5102A DAC, MAX17048 fuel gauge, RTC, power board, headphone detect, and validation programs.
2. Core OS runtime: boot, recovery, settings store, service scheduling, logging, power states, crash recovery, and safe shutdown.
3. UI and navigation: foreground SHAeR shell, stack navigation, settings, display renderer, simulator, and hardware display diagnostics.
4. Local library and audio: SD scan, SQLite library, GStreamer/local playback, volume, gapless, crossfade, ReplayGain.
5. Connectivity: Wi-Fi/Bluetooth service shape, then Spotify Connect once local playback is solid.
6. Emotional layers: voice archive, notifications, companion app, secret features, and the six theme worlds.
7. Physical product: breadboard validation, perfboard translation, enclosure validation, serviceability, and final battery/performance profiling.

## Important Wiring Conflict To Resolve Before Soldering

The final Obsidian Doc 21 says GPIO0/GPIO1 are reserved and GPIO2/GPIO3 are I2C, but later in the same file assigns LEFT to GPIO1 and RIGHT to GPIO2. That conflicts with both the reserved/I2C rules and the repo's `docs/GPIO_MASTER_TABLE.md`.

Until the architecture docs are corrected, use `docs/GPIO_MASTER_TABLE.md` as the code-facing source of truth for firmware pin maps. Do not wire buttons to GPIO1 or GPIO2.

## Next Step

Breadboard-validate Layer 3 on the Raspberry Pi:

```bash
cd /opt/shaer
make pi
sudo make install
sudo systemctl restart shaer
```

Then run the relevant diagnostics:

```bash
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

Layer 3 should not be tagged complete until these pass on the actual hardware and settings persistence survives a reboot.
