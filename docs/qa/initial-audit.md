# SHAeR Initial QA Audit

Date: 2026-08-04

This audit records the repository map and baseline command results before broad QA repairs. Hardware validation has not been performed.

## Repository Map

Canonical workspace: `/Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified`

The top-level folder is not a Git repository in this environment.

Primary source-of-truth documents:

- `README.md`
- `NEXT_CHAT_START_HERE.md`
- `INTEGRATION_MAP.md`
- `PROVENANCE.md`
- `pi_runtime/outputs/docs/FIRMWARE_CORE_ARCHITECTURE.md`
- `native_firmware/docs/SYSTEM_ARCHITECTURE.md`
- `native_firmware/docs/STATE_MACHINE.md`

Major areas:

- `native_firmware/`: C++ foreground firmware, HAL, simulator, storage, renderer, settings, tests, and Pi deployment scripts.
- `desktop_preview/`: laptop/browser renderer for the native firmware preview backend.
- `pi_runtime/outputs/shaer_pi_os/`: most complete Raspberry Pi browser runtime, GPIO/test-input server, shared JS state/navigation, and OS overlays.
- `pi_runtime/outputs/shaer_*`: eight Pi runtime themes and the companion/backend bundles.
- `pi_runtime/outputs/theme_validation/`: browser validation scripts for theme layout and navigation.
- `pi_runtime/outputs/shaer_companion/`: shared browser/Android/Windows PWA companion app.
- `pi_runtime/outputs/shaer_backend/`: Python backend services, diagnostics, HAL adapters, music, Spotify, recording, and tests.
- `apps/desktop/`: separate Tauri/Vite desktop app using shared device contracts and mock-device support.
- `packages/shared/src/`: shared TypeScript contracts.

## Identified Entry Points

Device UI:

- Native firmware UI: `native_firmware/firmware/ui/ui_framework.cpp`
- Pi runtime UI shell/state: `pi_runtime/outputs/shaer_pi_os/firmware-core.js`
- Pi runtime server: `pi_runtime/outputs/shaer_pi_os/server.py`

Desktop/laptop preview:

- Command: `npm run dev`
- Server: `desktop_preview/server.js`
- Browser interpreter: `desktop_preview/public/app.js`
- Native preview backend target: `native_firmware/build/shaer_preview_backend`

Companion app:

- Pi runtime companion app: `pi_runtime/outputs/shaer_companion/src/companion.js`
- Companion HTML shell: `pi_runtime/outputs/shaer_companion/index.html`
- Desktop app shell: `apps/desktop/src/main.tsx`

Backend/API server:

- Pi runtime server and device API: `pi_runtime/outputs/shaer_pi_os/server.py`
- Backend diagnostics/services: `pi_runtime/outputs/shaer_backend/`
- Native companion backend slice: `native_firmware/services/companion_backend/app.py`

Hardware abstraction:

- Native HAL: `native_firmware/firmware/hal/`
- Pi runtime HAL adapters: `pi_runtime/outputs/shaer_backend/shaer_hal/`

Theme directories:

- `pi_runtime/outputs/shaer_base_dark`
- `pi_runtime/outputs/shaer_base_light`
- `pi_runtime/outputs/shaer_dark_archive`
- `pi_runtime/outputs/shaer_bombay_ticket`
- `pi_runtime/outputs/shaer_japanese_punk`
- `pi_runtime/outputs/shaer_windows_xp`
- `pi_runtime/outputs/shaer_ghibli_garden`
- `pi_runtime/outputs/shaer_indian_print`
- Native theme assets: `native_firmware/themes/`, `native_firmware/assets/themes/`

Music library/indexing:

- Native storage: `native_firmware/firmware/storage/music_library_store.cpp`
- Pi backend library: `pi_runtime/outputs/shaer_backend/shaer_music/library.py`
- Native sync service: `native_firmware/services/sync/sync_library.py`
- Runtime JS store: `pi_runtime/outputs/shaer_pi_os/music-store.js`

Playlist and queue logic:

- Pi backend music providers: `pi_runtime/outputs/shaer_backend/shaer_music/providers.py`
- Companion music store/model normalizers: `pi_runtime/outputs/shaer_companion/src/core/`

Spotify integration:

- Pi backend Spotify services: `pi_runtime/outputs/shaer_backend/shaer_music/spotify*.py`
- Native service slice: `native_firmware/services/spotify/spotify_service.py`
- Pi setup doc: `pi_runtime/outputs/shaer_pi_os/LAYER12_SPOTIFY.md`

Pairing and sync:

- Companion protocol docs: `pi_runtime/outputs/docs/COMPANION_PROTOCOL.md`
- Companion source: `pi_runtime/outputs/shaer_companion/src/core/`
- Sync service: `native_firmware/services/sync/sync_library.py`

Mock-device mode:

- Desktop app mock device: `apps/desktop/src/core/mockDevice.ts`
- Pi runtime test input endpoint requires `--allow-test-input`.
- Companion app still needs capability-classified mock/real commands as part of later QA phases.

## Commands Used

Baseline commands run before broad edits:

```bash
npm test
make -C native_firmware -j1 check
npm test
npm run build:web
npm test
npm run typecheck
npm run build
node pi_runtime/outputs/theme_validation/settings-navigation-check.mjs
node pi_runtime/outputs/theme_validation/ui-correction-check.mjs
PYTHONPATH=. python3 -m unittest discover -s tests
```

The companion commands were run from `pi_runtime/outputs/shaer_companion`.
The desktop app commands were run from `apps/desktop`.
The backend unittest command was run from `pi_runtime/outputs/shaer_backend`.

## Existing Test Results

Passed:

- Top-level `npm test`: passed. It rebuilt the native preview target when needed and `desktop_preview/test_backend.js` reported `desktop preview backend passed (4 firmware frames, procedural themed primitives present)`.
- `make -C native_firmware -j1 check`: passed.
- `pi_runtime/outputs/shaer_companion` `npm test`: passed 13 Node tests.
- `pi_runtime/outputs/shaer_companion` `npm run build:web`: passed and built `dist`.
- `pi_runtime/outputs/shaer_backend` unittest discovery: passed 44 tests.

Warnings:

- `make -C native_firmware -j1 check` emitted many Python `ResourceWarning: unclosed database` warnings during `python3 -m unittest discover -s companion_app/tests`, although the command exited 0.

Failed or blocked:

- `node pi_runtime/outputs/theme_validation/settings-navigation-check.mjs`: failed because `shaer_pi_os/server.py` could not bind a local HTTP server in this sandbox: `PermissionError: [Errno 1] Operation not permitted`, followed by `SHAeR server did not become ready at http://127.0.0.1:8790`.
- `node pi_runtime/outputs/theme_validation/ui-correction-check.mjs`: failed for the same local server bind permission issue.
- `apps/desktop` `npm test`, `npm run typecheck`, and `npm run build` emitted startup lines but no useful output after several minutes in this environment; they were interrupted with Ctrl-C and should be rerun in a normal local shell before release claims.

## Build Failures

No product build failure was observed in commands that completed.

Environment-blocked/inconclusive build result:

- `apps/desktop` Vite build did not complete before interruption after prolonged silence.

## Missing Dependencies

No missing dependency was observed for the completed checks.

Potential environment limitations:

- Local server socket binding is blocked in this sandbox, preventing Playwright/browser theme validation scripts from starting the Pi runtime server.
- Process inspection with `ps`/`pgrep` is blocked, so long-running desktop app command state could not be independently inspected.

## Known Limitations

- The workspace is a combined handoff, not one production executable. `native_firmware` is the long-term foreground runtime, while `pi_runtime/outputs` remains the most complete runnable Pi prototype and theme oracle.
- The top-level workspace is not currently a Git checkout in this environment, so commit hash and dirty status cannot be recorded from the top-level directory.
- Existing theme validation artifacts are historical until the browser checks can be rerun successfully in an environment that allows local server binding.
- Desktop app tests/typecheck/build are inconclusive from this run.

## Hardware-Only Validation

The following cannot be honestly validated from laptop-only simulation:

- Raspberry Pi Zero 2 W boot behavior and thermal/power stability.
- 240x320 physical display color, refresh, clipping, crack, and panel-specific rendering behavior.
- Rotary encoder electrical behavior, GPIO polarity, debounce, OK/Back physical button behavior, and power switch behavior.
- Audio output, left/right channel verification, noise floor, headphone unplug/replug, and hardware volume behavior.
- Wi-Fi and Bluetooth radio behavior on the Pi.
- Spotify Connect against the physical device network environment.
- Companion pairing requiring physical SHAeR approval.
- Storage reliability on the target medium, reboot persistence, charging, battery operation, and mechanical enclosure fit.
