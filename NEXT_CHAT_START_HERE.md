# Next Chat Start Here

For the complete product, architecture, UI, verification, release, and remaining-work context, use `FULL_CONTEXT_PROMPT.md` as the primary continuation prompt.

## Canonical Path

Use only:

`/Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified`

The dated source folders are provenance snapshots. Do not continue implementing separately in either of them.

## What Is Already Built

### Native firmware

`native_firmware/` contains the C++17 SHAeR foreground runtime:

- event bus, scheduler, app state, screen manager, resource manager, and notifications
- desktop and Raspberry Pi HAL boundaries
- boot/recovery, structured logging, SQLite settings/library storage
- audio playback foundation
- UI framework, theme engine, native simulator, diagnostics, and systemd/install scripts
- native tests for runtime, navigation, settings, themes, renderer, memory, performance, audio, fonts, and transitions

This source came from the complete working tree, including its uncommitted Layer 3/theme work. Generated databases, logs, old builds, releases, and its nested `.git` directory were intentionally excluded.

### Pi runtime and product layers

`pi_runtime/` contains firmware version `0.16.0` and companion version `0.17.0`:

- six visual themes and reference assets
- one shared JavaScript firmware state/navigation core
- Python music library, Spotify PKCE/Connect, companion protocol, recording archive, backup/restore, and security hardening
- one shared companion frontend/API/model layer packaged as an Android APK, Windows-installable PWA, and browser fallback
- Android Keystore credential protection, Android NSD discovery, encrypted browser credential storage, linked-device revocation, and future-compatible owner roles
- GPIO input HAL and source-neutral provider contracts
- theme validation harness and Raspberry Pi deployment/service files
- current Pi bundle builder

## Verified Before Handoff

- Native source previously passed `make check`; verify again from this unified copy.
- Shared companion JavaScript: 9 tests passed.
- Pi runtime: 28 backend tests passed.
- Pi runtime: 18 companion/security tests passed, including loopback HTTP flows.
- Pi runtime: 35 diagnostics passed.
- Theme harness: 78 rendered states passed with no layout, state-contract, parity, or console errors.

## First Task For The Next Chat

Do not add another feature first. Integrate the runtime boundary in this order:

1. Use the native C++ state machine and input manager as the sole product-state authority.
2. Convert the JavaScript firmware-core transition table and invariants into native C++ tests, then retire duplicate JS behavioral state on the physical device.
3. Run Spotify, recording, companion, and indexing as supervised services behind narrow source-neutral IPC/provider interfaces.
4. Treat the July theme renderings and validation screenshots as the visual oracle; make the native renderer reproduce them.
5. Converge settings and SQLite schemas so each value has one owner and one persistence path.
6. Produce one Raspberry Pi service graph with one foreground UI process.

## Useful Commands

```bash
cd /Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
./test_all.sh
./build_all.sh
```

Native simulator:

```bash
./native_firmware/build/shaer_simulator
```

Current complete prototype server:

```bash
python3 pi_runtime/outputs/shaer_pi_os/server.py \
  --host 127.0.0.1 --port 8775 --theme shaer_dark_archive --allow-test-input
```

Then open:

`http://127.0.0.1:8775/shaer_dark_archive/?mode=device`

## Hardware-Only Gates Still Open

- Raspberry Pi display, encoder/buttons, DAC, battery, charging, Wi-Fi, Bluetooth, and microphone acceptance
- power-loss tests during database, playlist, recording, restore, and indexing writes
- nearly-full and corrupt SD-card cases
- 20,000-track library performance
- ten boot/shutdown/Spotify/theme stress loops
- 72-hour RAM/CPU/service soak
- install and exercise `SHAeR-Companion-0.17.0-debug.apk` on a real Android phone or tablet
- install the PWA through Edge and verify reconnect/uninstall behavior on a real Windows laptop
- verify Android discovery, physical OK pairing, reboot reconnection, real Spotify controls, and credential revocation against the physical SHAeR
- replace authenticated local HTTP with an encrypted and authenticated transport before public release
- configure protected Android release signing; the current APK carries a debug signature

Do not describe these as passed until they are run on the actual device.
