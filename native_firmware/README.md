# आदि Vasi OS / SHAeR

![Version](https://img.shields.io/badge/version-0.1.0--alpha.1-indigo)
![Target](https://img.shields.io/badge/target-Raspberry%20Pi%20Zero%202%20W-darkgreen)
![License](https://img.shields.io/badge/license-private-lightgrey)

`आदि Vasi OS` is the embedded operating environment for SHAeR. It runs on top of Raspberry Pi OS Lite, but the user should never see Linux. From power-on to shutdown, every visible pixel belongs to SHAeR.

## Product Contract

Cold boot:

```text
Power button
-> Linux boots silently
-> आदि Vasi OS claims the foreground
-> Libra constellation boot animation
-> Loading D: Drive...
-> mhm mhm
-> hardware/services initialize behind the animation
-> Home
```

Cold boot always starts at Home. It never resumes directly into Now Playing.

Home options:

```text
Spotify Connect
Local Library
Voice Archive
```

Sleep is different from cold boot: sleep keeps Linux alive and can wake instantly.

## What Linux Is Allowed To Do

Linux exists only as the hardware and process substrate:

- drivers
- filesystem
- ALSA/audio substrate
- BlueZ/Bluetooth substrate
- Wi-Fi/network substrate
- systemd supervision

Linux must not show a desktop, terminal, login prompt, mouse cursor, boot console, or Raspberry Pi-branded surface.

## Current Build

```bash
make
make check
./build/shaer_simulator
```

Root build targets:

```bash
make            # desktop simulator and test binaries
make check      # Mac/dev validation: firmware tests, simulator checks, companion tests
make simulator  # desktop foreground binary
make pi         # Raspberry Pi foreground binary; run this on Raspberry Pi OS/Linux
make install    # Raspberry Pi install only; run with sudo on the Pi, not on macOS
make package    # create releases/shaer-<version>.tar.gz
make release    # run checks and create release archive
make clean
```

On a Mac, do not run `sudo make install`. The Mac is for development and simulation:

```bash
make check
make simulator
./build/shaer_simulator
```

`make pi` and `sudo make install` are for the Raspberry Pi after the repository has been cloned or pulled there.

The reference visual renderer is:

```text
simulator/web/index.html
```

It shows all six themes across Boot, Home, Library, Now Playing, Settings, Spotify/Wi-Fi/Bluetooth popups, Charging, Sleep, and Battery Saver. It includes Libra constellation boot, old-device timestamp/date footer, 12-hour clock, battery percentage, pixel-like typography, and theme-specific animation language.

## Companion App

Run the desktop manager:

```bash
python3 companion_app/run_companion.py --ui web --data-root .companion_data
```

The companion currently supports the non-hardware management surface:

- Library import
- Metadata viewer
- Playlist editor
- Duplicate finder
- Theme installer
- Device settings editor
- Firmware package registration/staging
- Voice note index
- SD-card sync preview/sync

## Git-First Deployment

The repository is the single source of truth. Do not manually copy source files onto the SD card.

Normal development and deployment flow:

```text
Mac
-> Git commit
-> private GitHub repository
-> Raspberry Pi git pull
-> make
-> sudo make install
-> sudo systemctl restart shaer
```

First-time Raspberry Pi setup:

```bash
sudo ./scripts/pi_first_time_setup.sh
sudo git clone <private-github-url> /opt/shaer
cd /opt/shaer
make pi
sudo make install
sudo systemctl restart shaer
```

Normal Pi update:

```bash
cd /opt/shaer
sudo ./update.sh
```

`update.sh` backs up `/var/lib/shaer`, pulls the latest Git revision, compiles, installs, restarts `shaer.service`, and rolls back if the build or service restart fails.

## Project Layout

```text
README.md        Project entry point
LICENSE          Private project license
VERSION          Semantic firmware/app version
CHANGELOG.md     Release history
docs/            OS, hardware, architecture, workflow, recovery, power, state machine
firmware/        C++ आदि Vasi OS foreground runtime
ui/              Shared UI package boundary
themes/          Theme package boundary; active packs are in assets/themes
services/        Python/background services
database/        Schema and migration notes
sync_app/        Future sync-specific package boundary
simulator/       Browser-based visual simulator
assets/          Theme packs and future bundled assets
scripts/         Pi OS Lite setup, install, update, packaging
tests/           C++ and Python checks
tools/           Repository maintenance notes
build/           Generated build output
releases/        Generated release archives
systemd/         Foreground OS service units
```

## Repository Rules

- Development follows `docs/LAYERED_MILESTONE_WORKFLOW.md`.
- The active layer is tracked in `docs/CURRENT_MILESTONE.md`.
- Every layer has Architecture, Implementation, Software Validation, Breadboard Hardware Validation, and Completion checkpoints.
- `main` is releasable.
- `develop` is integration.
- Feature work happens on `feature/<name>`.
- Hardware bring-up happens on `hardware/<name>`.
- Releases are tagged as `vMAJOR.MINOR.PATCH[-prerelease]`.
- Commits use Conventional Commits.

See `docs/LAYERED_MILESTONE_WORKFLOW.md`, `docs/DEVELOPMENT_WORKFLOW.md`, `docs/REPOSITORY_STRATEGY.md`, `docs/DEPLOYMENT.md`, and `docs/RELEASE_PROCESS.md`.

## Do Not Build Without Hardware

These stay deferred until the Raspberry Pi is physically available:

- GPIO driver changes
- SPI display driver changes
- I2C battery-gauge driver changes
- DAC driver changes
- Bluetooth driver changes beyond service/API shape

Everything else can keep moving in the simulator, companion app, tests, and OS docs.
