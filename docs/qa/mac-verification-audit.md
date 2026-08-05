# SHAeR Mac Verification Audit

Date: 2026-08-05

Scope: Phase 1 repository audit only. No broad implementation changes were made. Hardware functionality was not tested.

## Repository Identity

- Opened repository/workspace root: `/Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified`
- Git status: this directory is not a Git repository in the current checkout. `git rev-parse --show-toplevel` returns `fatal: not a git repository`.
- Current branch: unavailable because no `.git` directory exists under this workspace.
- GitHub remote: unavailable for the same reason.
- Uncommitted/untracked files: cannot be distinguished from tracked files because Git metadata is absent.
- Important code outside Git tracking: yes, effectively the whole opened SHAeR workspace is outside visible Git tracking in this environment.

## Source Map

`native_firmware/`

- Long-term foreground firmware foundation.
- Owns C++ app lifecycle, HAL, core state, audio engine, settings catalog, storage stores, UI primitive renderer, simulator, systemd units, native install/update scripts, and native tests.

`desktop_preview/`

- Browser preview for the native firmware renderer.
- `desktop_preview/server.js` serves the preview.
- `desktop_preview/public/app.js` interprets serialized `UiCommand` frames from the native preview backend.
- This is not a separate product UI; it is a renderer preview path.

`pi_runtime/outputs/shaer_pi_os/`

- Most complete runnable Raspberry Pi browser prototype.
- Owns Pi HTTP server, GPIO/test-input bridge, shared browser navigation/state, overlays, Pi service files, Spotify setup helpers, BLE discovery installer, and deploy helper.
- Key files: `server.py`, `firmware-core.js`, `hardware-bridge.js`, `music-store.js`, `system-overlays.css`.

Companion applications

- Main Pi-runtime companion PWA/Android/Windows source: `pi_runtime/outputs/shaer_companion/`.
- Separate desktop app concept: `apps/desktop/`, with React/Vite/Tauri and shared contracts.
- Older/native Python companion code remains in `native_firmware/companion_app/`.

Shared theme/UI assets

- Runtime themes: `pi_runtime/outputs/shaer_base_dark`, `shaer_base_light`, `shaer_dark_archive`, `shaer_bombay_ticket`, `shaer_japanese_punk`, `shaer_windows_xp`, `shaer_ghibli_garden`, `shaer_indian_print`.
- Native theme assets also exist under `native_firmware/themes/` and `native_firmware/assets/themes/`.
- Theme validators and screenshot artifacts live under `pi_runtime/outputs/theme_validation/`.

Tests, validators, deployment

- Top-level host validator: `test_all.sh`.
- Top-level release packager: `build_all.sh`.
- Native test/build entry: `native_firmware/Makefile`.
- Pi runtime bundle builder: `pi_runtime/build_pi_bundle.sh`.
- Pi deploy helper: `pi_runtime/outputs/shaer_pi_os/deploy_to_pi.sh`.
- CI currently covers only Windows desktop release in `.github/workflows/desktop-windows-release.yml`.

## Real Source Of Truth

- UI: split. Native C++ UI primitives are long-term source of truth; Pi runtime themes are current visual/product behavior oracle.
- Navigation/state: split. Native firmware core is the intended owner; current complete browser behavior lives in `pi_runtime/outputs/shaer_pi_os/firmware-core.js`.
- Hardware input: native HAL in `native_firmware/firmware/hal/`; Pi runtime GPIO/test input in `pi_runtime/outputs/shaer_pi_os/server.py` and backend HAL adapters in `shaer_backend/shaer_hal/`.
- Audio: native playback engine in `native_firmware/firmware/audio/`; Pi diagnostics/services cover contract-level audio, Spotify, and local playback paths.
- Settings/persistence: native settings catalog/store in `native_firmware/firmware/settings/` and `native_firmware/firmware/storage/`; Pi runtime also has device API/service persistence.
- Themes: current visual oracle is `pi_runtime/outputs/shaer_*`; native renderer migration assets live under `native_firmware`.
- Pi startup: `pi_runtime/outputs/shaer_pi_os/shaer-pi-os.service`, `start_theme.sh`, and install layer scripts for the browser runtime; `native_firmware/systemd/` for the native service path.

## Assets, Secrets, Generated Files

- Large generated/build files are present in the workspace:
  - `apps/desktop/src-tauri/target` is about 1.0G.
  - `pi_runtime/outputs/shaer_companion.zip` is about 31M.
  - `native_firmware/build` is about 3.5M.
- No `.env.example` exists in the audited paths.
- Required lockfiles exist for npm/Rust areas: top-level `package-lock.json`, `apps/desktop/package-lock.json`, `apps/desktop/src-tauri/Cargo.lock`, `pi_runtime/outputs/shaer_companion/package-lock.json`.
- Python has `pi_runtime/pyproject.toml` but no pinned requirements lockfile for a clean Pi install.
- Secret-looking source search found credential-store code and generated copies, but no obvious `.env`, `.pem`, or `.key` files in the audited results.
- `.gitignore` cannot be evaluated at the top level because no top-level Git repo exists. Subdirectory ignore files exclude runtime databases, zips/tarballs, caches, secrets, and `pi_runtime/outputs/theme_validation/artifacts`. That is reasonable for generated artifacts, but required baseline/reference assets need review once Git tracking is restored.

## Command Results

Passed:

- `npm test` from workspace root: passed; desktop preview backend reported 4 firmware frames and procedural themed primitives.
- `make -C native_firmware -j1 check`: passed native C++ tests, web simulator checks, Python companion tests, and shell syntax checks.
- `npm test` in `pi_runtime/outputs/shaer_companion`: passed 13 JS contract/model tests.
- `npm run build:web` in `pi_runtime/outputs/shaer_companion`: passed and built `dist`.
- `PYTHONPATH=. python3 -m unittest discover -s tests` in `pi_runtime/outputs/shaer_backend`: passed 44 tests.
- `npm test` in `apps/desktop`: passed 1 Vitest file, 4 tests. It was slow: about 103 seconds.
- `node pi_runtime/outputs/theme_validation/settings-navigation-check.mjs` unsandboxed: passed, 8 themes, 9 settings pages, 0 errors.
- `node pi_runtime/outputs/theme_validation/ui-correction-check.mjs` unsandboxed: passed, 0 errors; screenshots generated under `pi_runtime/outputs/theme_validation/artifacts/ui-correction`.
- `npm run package:pwa` in `pi_runtime/outputs/shaer_companion`: passed and produced a PWA zip plus SHA-256.

Warnings and inconclusive:

- `make -C native_firmware -j1 check` repeatedly emits Python `ResourceWarning: unclosed database` warnings during native companion unittest discovery, though exit code is 0.
- `node pi_runtime/outputs/theme_validation/settings-navigation-check.mjs` and `ui-correction-check.mjs` fail inside the sandbox because `server.py` cannot bind localhost; they pass when run unsandboxed.
- `bash test_all.sh` was run unsandboxed. Native, backend, companion/security, diagnostics, and JS contract sections passed, but final `theme-validation.mjs --no-baseline` ran silently for several minutes and was interrupted. On interrupt it reported `page.evaluate: Target page, context or browser has been closed`.
- `npm run typecheck` in `apps/desktop` emitted no result after several minutes and was interrupted.
- `npm run build` in `apps/desktop` emitted Vite startup/transforming output, then no result after several minutes and was interrupted.
- `make -C native_firmware package` was still archiving after several minutes and was interrupted.
- `bash pi_runtime/build_pi_bundle.sh` emitted no result after several minutes and was interrupted.

## Prioritized Findings

### BLOCKER

1. No Git repository metadata is present in the opened SHAeR workspace, so branch, remote, tracked/untracked files, reproducibility, and small recoverable checkpoints cannot be verified.
2. `test_all.sh` is not currently a reliable unattended Mac verification gate because its final `theme-validation.mjs --no-baseline` step hangs silently and lacks a bounded timeout/failure report.
3. Clean Pi deployment is not yet repeatable from this audit: the Pi bundle builder did not complete in a reasonable audit window, `.env.example` is missing, Python dependencies are not fully pinned, and no clean-clone rehearsal can be proven without Git metadata.

### MAJOR

1. Source of truth is split between native firmware and Pi runtime. This is documented, but it means the full product is not yet one production executable.
2. Desktop app `typecheck` and Vite `build` did not terminate in this environment, so the desktop companion cannot yet be claimed clean-buildable on Mac.
3. Companion package version drift exists: `package.json` says `0.19.0`, while `tools/package-pwa.mjs` outputs `shaer-companion-pwa-0.18.0.zip`.
4. Native package and Pi bundle scripts appear too quiet/slow for reliable deployment rehearsal. They need progress, exclusions, and time bounds before being trusted as release commands.
5. The all-tests diagnostics report several hardware-related checks as contract/pending, which is correct, but the release report should separate these from real hardware pass/fail.

### VISUAL

1. Focused 240x320 settings-navigation and UI correction checks pass across all eight themes when localhost binding is allowed.
2. Existing bounds/layout validators are not enough for the requested approved-reference visual comparison. The current audit confirms screenshot artifacts exist, but not that every canonical state has been visually reviewed against approved references.
3. Theme validation artifacts are generated under an ignored artifacts directory; this is fine for transient screenshots but needs a clear baseline/reference policy.

### HARDWARE-ONLY

1. Physical display color, refresh, clipping, crack, and exact panel behavior remain untested.
2. Rotary encoder electrical behavior, OK/Back buttons, GPIO polarity, debounce, and power switch remain untested.
3. DAC/headphone audio, channel balance, noise, volume, and unplug/replug remain untested.
4. Wi-Fi/Bluetooth radio behavior, Spotify Connect on-device, companion physical pairing approval, charging, battery, storage medium reliability, reboot persistence, thermals, and enclosure fit remain untested.

### CLEANUP

1. Generated/vendor/build artifacts are mixed into the opened workspace, including `node_modules`, Tauri `target`, companion `dist`, Android intermediates, releases, and theme artifacts.
2. Repeated Python unclosed SQLite warnings should be fixed or suppressed by proper cleanup before release QA.
3. Generated duplicate companion icons with names like `shaer-192 2.png` and `shaer-512 2.png` should be reviewed.
4. The workspace needs a top-level ignore/tracking policy once Git is restored.

## Audit Recommendation

Do not proceed to broad UI changes yet. First restore or identify the real Git checkout, then make the Mac verification gate deterministic:

1. Restore Git metadata or move this workspace into a real Git repository with the correct GitHub remote.
2. Add timeouts/progress to `test_all.sh`, `theme-validation.mjs`, native package, and Pi bundle scripts.
3. Fix or document the desktop app `typecheck`/build hang.
4. Resolve companion version drift.
5. Add `.env.example` and dependency-lock policy for Pi deployment.
6. Only then begin Phase 2 laptop UI harness work.
