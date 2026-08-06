# Recovery Branch Review

Branch: `recovery/software-qa-clean-state`

Preserved checkpoint: `158ea7bc7945cb7dfca0e110d38ccbf4ebbe4504`

Remote: `https://github.com/Rishikakaps/SHAeR.git`

## Publication

The checkpoint was pushed to `origin/recovery/software-qa-clean-state`. It was not pushed to `main`, force-pushed, tagged, merged, or used to delete remote history.

Pre-push path scan of `origin/main..158ea7b` found no `.env` files, database files, credential/key-looking files, `node_modules`, caches, build directories, QA screenshot artifacts, release binaries, or machine-private dotfiles.

## Added File Audit

Compared with GitHub `main`, checkpoint `158ea7b` introduces 401 added files and 6 modified files.

Approximate added-file groups:

| Group | Count | Notes |
| --- | ---: | --- |
| Essential source code | 113 | Native firmware, Python backend, companion app, Pi runtime server/HAL, theme adapters. |
| UI/theme assets | 147 | Approved theme images, baselines, CSS, fonts, and reference imagery. Required for offline rendering. |
| Tests and validators | 3 | Top-level classifier count only; many test files are under source-owned directories and counted above. |
| Documentation | 81 | Handoff docs, architecture notes, QA reports, companion status, Pi/runtime docs. |
| Installation/deployment files | 11 | Service units, shell installers, deploy script, package manifests, checksum files. |
| Suspicious/generated/other | 46 | Mostly app entrypoints/config JSON/checksum-only release manifests; release checksum files are present without APK/ZIP binaries. |

Largest added files are UI/reference assets, not credentials or build outputs. The largest is `pi_runtime/outputs/shaer_windows_xp/assets/xp-reference-base.png` at 424 KB. Several Bombay Ticket, Ghibli Garden, Indian Print, and validation-baseline PNGs are 100-212 KB and are expected offline assets or visual references.

## Modified Existing Files

The six files modified by checkpoint `158ea7b` relative to GitHub `main` are:

- `README.md`
- `package-lock.json`
- `package.json`
- `pi_runtime/outputs/shaer_companion/package-lock.json`
- `pi_runtime/outputs/shaer_pi_os/hardware-bridge.js`
- `pi_runtime/outputs/shaer_pi_os/system-overlays.css`

## Major Subsystems Introduced

- Native firmware simulator, renderer, HAL, settings/library persistence, test suite, and Pi bring-up scripts.
- Browser/Python Pi runtime with eight theme worlds and shared firmware-core navigation/state.
- Companion app source for browser/PWA/Android flows.
- Python backend services for local library, Spotify, recording archive, diagnostics, pairing, settings, backup/restore, and updates.
- Theme validation and 240x320 Mac QA harness.
- Recovery/audit documentation and runtime architecture notes.

## Source-of-Truth Risks Before Merge

- UI source exists in multiple surfaces: native firmware UI framework, browser Pi runtime themes, `desktop_preview`, and `apps/desktop`. The Pi runtime currently contains the most complete device behavior; native firmware remains the long-term foreground runtime foundation.
- Startup/service source exists in multiple places: old layer installers, checked-in service defaults, native `systemd` files, and the new Bookworm bootstrap. Use `bootstrap_bookworm.sh` for the next Pi pass.
- Companion code exists in both `native_firmware/companion_app` and `pi_runtime/outputs/shaer_companion`; the latter is the current shared browser/Android/Windows-PWA companion.
- Release checksum files are present without APK/ZIP binaries. That is good for avoiding release binaries in Git, but the checksums alone are obsolete unless the matching releases are reproduced.
- Visual baselines are committed and required for comparison, but the full validator currently reports baseline drift after regeneration.

Merging this branch into `main` would substantially replace GitHub behavior by introducing the combined SHAeR handoff workspace. It should be reviewed as a large source recovery/import branch, not a small patch.

## Required For Clean Clone

Required non-secret files include source code, package manifests/lockfiles, theme assets, fonts, visual baselines, service templates/defaults, Python project metadata, and docs describing unresolved hardware gates.

Generated files that should remain ignored include `node_modules/`, `apps/desktop/dist/`, `pi_runtime/outputs/shaer_companion/dist/`, `native_firmware/build/`, `__pycache__/`, `*.egg-info/`, and `pi_runtime/outputs/theme_validation/artifacts/`.

No credentials, `.env` files, databases, private keys, release binaries, QA screenshots, or machine-specific local paths should be committed.

## NPM Audit

Root workspace audit: 0 vulnerabilities.

Desktop app audit: 0 vulnerabilities.

Companion app before remediation:

- `brace-expansion`, high severity, path `@capacitor/cli -> glob -> minimatch -> brace-expansion`, dev/build tooling exposure.
- `tar`, moderate severity, path `@capacitor/cli -> tar`, dev/build tooling exposure.

Remediation applied without `npm audit fix --force`: lockfile-only transitive update to `brace-expansion@5.0.9` and `tar@7.5.22`. Companion audit now reports 0 vulnerabilities.
