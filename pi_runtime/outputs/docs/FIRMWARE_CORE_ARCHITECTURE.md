# SHAeR Firmware Core Architecture

## Runtime Boundary

SHAeR now has one behavioral runtime: `shaer_pi_os/firmware-core.js`.

The core owns:

- page state, transition validation, history, and Back behavior
- rotary selection and OK/Back keyboard equivalents
- playback and recording mutual exclusion
- current-track bounds and playing-without-a-track rejection
- persistence, loading timing, settings capability gating, and bounded structured logs
- canonical state names shared by all themes

Each theme module now contains only fixture state, visual render functions, aliases, and a single `SHAeRFirmware.mount(...)` call. Theme modules do not register key handlers, own timers, manage history, or write persistent state.

`hardware-bridge.js` remains the shared adapter between browser events and device services. It owns system overlays, Spotify polling/control, recording control, device events, and performance telemetry.

## State Contract

Canonical pages are:

`boot`, `home`, `loading`, `library`, `album`, `now-playing`, `queue`, `recordings`, `settings`, `charging`, `about`, and `error`.

All normal transitions are listed in the frozen `transitions` table in `firmware-core.js`. Unknown and illegal transitions fail closed, display an error, and write a structured log entry.

Back follows one rule:

1. Return to the most recent valid page in the bounded history.
2. If history is empty and the current page is not Home, return Home.
3. At Home, remain at Home.

Archive Dark, Bombay Ticket, and Japanese Punk provide an Album page. Windows XP, Ghibli Garden, and Indian Print intentionally preserve their reference flow and route Album capability back to Library. This difference is declared in the firmware snapshot and validated by the theme harness.

## State Invariants

The core normalizes state before persistence, rendering, and snapshots:

- `playing` is false when no track exists.
- current track index always falls inside the song list.
- playback stops before recording becomes active.
- history contains only pages supplied by the mounted theme and is bounded to 24 entries.
- exactly one visible encoder target is selected when a page has controls.
- animations and loading timers cannot prevent input processing.

Provider events that claim playback without a title or URI are rejected and logged as `rejected-impossible-playback`.

## Provider Boundary

`shaer_music/providers.py` defines source-neutral `MediaItem`, `ProviderCapabilities`, `LibraryProvider`, `PlaybackProvider`, and `ProviderRegistry` contracts. `LocalLibraryProvider` is the first concrete adapter. Spotify can move behind this contract without adding provider branches to themes.

The UI consumes source-neutral playback metadata from `shaer:playback`; Local and Spotify must bind to the same title, artist, album, artwork, progress, and controls.

## Hardware Boundary

`shaer_hal` defines display, audio, battery, network, Bluetooth, microphone, power, and input contracts. `GpioInputController` now owns all `gpiozero` imports, encoder callbacks, debounce, and short/long button interpretation. `server.py` receives semantic input events only.

`SimulatedInputController` drives the same callback contract in host tests.

## Settings Contract

Every visible setting must have firmware behavior. The UI currently exposes:

- Recorder: opens the recording system.
- About: opens a real firmware/version popup.
- Power: visible only when `/api/system/capabilities` reports hardware shutdown enabled.

All other design placeholders are hidden. Adding a setting requires its firmware behavior, persistence rule, capability declaration, and validation test in the same change.

## Storage and Recovery

Library and recording databases use WAL mode, FULL synchronous writes, a 5-second busy timeout, foreign keys, and bounded WAL checkpoints. Recording audio is finalized from partial files with atomic replacement; sidecars are fsynced and atomically replaced. Backup restore validates SQLite payloads and rolls back partially committed swaps.

Power-loss and storage-exhaustion tests still require physical media because host tests cannot reproduce Raspberry Pi filesystem and power behavior.

## Validation Gate

`theme_validation/theme-validation.mjs` renders 78 theme/state combinations and verifies geometry, bounds, clipping, popup overlap, Local/Spotify parity, core mounting, canonical transitions, Back, encoder selection, state invariants, settings capabilities, and console cleanliness.

Backend tests validate provider registration, source-neutral media, HAL input parity, and SQLite durability pragmas. Diagnostics statically reject theme-owned navigation controllers.

## Structured Logs

The core retains the latest 300 structured entries in `window.SHAeRLog`. Entries use category and event fields, with focused records for firmware mount, input, navigation, playback, storage failure, capability discovery, and invariant rejection. High-frequency rendering details are not logged.

## Hardware Acceptance Gates

The host build is not a substitute for hardware acceptance. Release approval still requires:

1. Ten cold boots and ten safe shutdowns.
2. Encoder response under 50 ms, including rapid direction reversal and switch bounce.
3. DAC, display, battery, charging, Wi-Fi, Bluetooth, and microphone disconnect/recovery tests.
4. Spotify transfer/reconnect and local playback loops.
5. Power loss during history, playlist, recording, and library writes.
6. Nearly-full SD card, corrupt media/artwork/database/backup, and a 20,000-track library.
7. A 72-hour soak with RAM plateau, low idle CPU, and no unrecovered service failure.

Failures must produce a bounded error state or unavailable capability, never an impossible playback state or process crash.
