# SHAeR Unified Workspace

This is the canonical SHAeR handoff workspace assembled from the two Codex builds:

- `native_firmware/`: the 26 June C++ foreground firmware, HAL, simulator, native renderer, local library/settings foundation, tests, hardware docs, and Raspberry Pi build/install scripts.
- `pi_runtime/`: the 4-13 July Python/browser Raspberry Pi runtime, six finished theme worlds, shared UI state machine, Spotify Connect, companion app, voice archive, security hardening, theme validation, and deployable Pi bundle.

Start all future SHAeR work from this directory:

```bash
cd /Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
```

Read these before editing:

1. `NEXT_CHAT_START_HERE.md`
2. `INTEGRATION_MAP.md`
3. `PROVENANCE.md`
4. `pi_runtime/outputs/docs/FIRMWARE_CORE_ARCHITECTURE.md`
5. `native_firmware/docs/SYSTEM_ARCHITECTURE.md`
6. `native_firmware/docs/STATE_MACHINE.md`

## Build And Companion Releases

Build the native simulator/tests and both release archives:

```bash
./build_all.sh
```

Run every host-side test:

```bash
./test_all.sh
```

Generated release artifacts are placed in `releases/`, including:

- `SHAeR-Companion-0.17.0-debug.apk`: installable Android acceptance build.
- `shaer-companion-pwa-0.17.0.zip`: shared Windows-installable PWA and browser fallback.
- SHA-256 checksum files for both companion packages.
- the Raspberry Pi runtime bundle, native firmware package, and unified source archive.

The Android APK uses a standard debug signature. It is suitable for physical acceptance testing, not public distribution. A release signing key, encrypted local transport, and real Android/Windows hardware acceptance are still required before calling the companion a consumer release.

## Current Truth

This workspace is complete as a combined source handoff, but the two execution tracks are not yet one production executable.

- The native C++ track is the correct long-term foreground firmware foundation, but its original milestone stops at Layer 3.
- The Pi runtime contains the later product behavior through Spotify, the shared browser/Android/Windows-PWA companion, recording, six themes, and validation.
- The immediate integration task is to move the July service contracts and final theme behavior behind the native C++ runtime without creating a third implementation.

Until that integration is complete, `pi_runtime/shaer_pi_os_bundle.tar.gz` is the most complete runnable Raspberry Pi prototype and `native_firmware/build/shaer_simulator` is the compiled native firmware simulator. Companion release truth and remaining physical gates are recorded in `pi_runtime/outputs/shaer_companion/COMPANION_PRODUCT_STATUS.md`.
