# Layer 15 Status and Evidence

This ledger separates local contract evidence from Raspberry Pi acceptance evidence.

## Implemented

- Production GStreamer capture backend and explicit synthetic test backend.
- Start, pause, resume, stop, cancel, duration limit, and storage guard.
- Year/month WAV archive, atomic sidecars, dedicated SQLite index, and recovery journal.
- Recorder controls and recording library entry in all six themes.
- Rotary-friendly recent/favorite/month/year archive overlay.
- Source-neutral Now Playing metadata and inline WAV playback.
- Companion browse, rename, favorite, duplicate, move, delete, download, backup, and restore.
- Recording diagnostics and automated lifecycle/recovery/backup coverage.

## Local Evidence

Build commands:

```bash
python3 -m compileall -q outputs/shaer_backend outputs/shaer_companion outputs/shaer_pi_os
bash -n outputs/shaer_pi_os/install_layer15.sh outputs/shaer_pi_os/deploy_to_pi.sh
node --check outputs/shaer_pi_os/hardware-bridge.js
node --check outputs/shaer_companion/src/companion.js
node --check outputs/theme_validation/theme-validation.mjs
```

- Compiler warnings: 0 reported.
- Compiler errors: 0.
- Direct unit/protocol tests: 27 passed.
- Recording lifecycle/recovery tests: 5 passed within the 27-test run.
- Standalone diagnostics: 35 passed at contract level.
- Microphone and DAC diagnostic result: `PENDING` physical hardware, reported explicitly by the scripts.
- Localhost HTTP integration: `PENDING` because this execution sandbox rejected loopback binding before the server started.
- Fresh 78-state screenshot validation: `PENDING` because this execution environment rejected both browser process launch and direct local-file browser navigation. The harness now includes Recording as its thirteenth canonical state.

Test commands:

```bash
PYTHONPATH=outputs:outputs/shaer_backend python3 -m unittest -v \
  outputs/shaer_backend/tests/test_layer11_music.py \
  outputs/shaer_backend/tests/test_layer12_spotify.py \
  outputs/shaer_backend/tests/test_layer15_recording.py \
  outputs.shaer_companion.tests.test_layer14_companion.CompanionProtocolTests
PYTHONPATH=outputs/shaer_backend python3 outputs/shaer_backend/diagnostics/run_diagnostics.py
```

## Raspberry Pi Evidence

- Deployment: `PENDING`
- INMP441 initialization: `PENDING`
- Pause/resume capture: `PENDING`
- PCM5102 playback quality: `PENDING`
- Companion retrieval over Wi-Fi: `PENDING`
- Backup/restore on device: `PENDING`
- Forced power-loss recovery: `PENDING`
- Low-battery finalization: `PENDING`

## Git Evidence

Git commit: `UNAVAILABLE` because this workspace is not a Git repository.
