# SHAeR Desktop Feature Verification

Date: 2026-07-30

This ledger follows the vertical-slice rule: a feature is not complete because UI exists. A complete feature must reach the SHAeR backend, perform a real device/filesystem/system operation, read back the result, and show that verified result in the desktop app.

| Feature | UI implemented | Mock-backed | Real API | Real device operation | Read-back verified | Physical hardware verified |
| ------- | -------------: | ----------: | -------: | --------------------: | -----------------: | -------------------------: |
| Manual connection | Yes | No | Yes | Partial | Yes | No |
| Pairing | Yes | No | Yes | Partial | Partial | No |
| Persistent reconnection | Yes | No | Yes | No | Yes | No |
| Device status dashboard | Yes | No | Yes | Partial | Yes | No |
| Brightness setting | Yes | No | Partial | No | Partial | No |
| Active theme setting | Yes | No | Yes | Partial | Partial | No |
| Sleep timeout setting | Yes | No | Partial | No | Partial | No |
| Device name setting | Yes | No | Partial | No | Partial | No |
| Battery percentage visibility | Yes | No | Partial | No | Partial | No |
| Single MP3 upload | Yes | No | Yes | Partial | Partial | No |
| Library read-back after upload | Yes | No | Yes | Partial | Partial | No |
| Wi-Fi status | Yes | No | Yes | Yes | Yes | No |
| Wi-Fi scan | Yes | No | Yes | Yes | Yes | No |
| Wi-Fi connect | No | No | No | No | No | No |
| Bluetooth status | Yes | No | Yes | Yes | Yes | No |
| Bluetooth scan | Yes | No | Yes | Yes | Yes | No |
| Bluetooth pair/connect | No | No | No | No | No | No |
| Mock device mode | Yes | Yes | No | No | N/A | No |

## Endpoints Used Or Added

Existing endpoints used by the desktop app:

- `GET /api/v1/device/discovery`
- `POST /api/v1/pairing/start`
- `GET /api/v1/pairing/status`
- `GET /api/v1/dashboard`
- `GET /api/v1/device/capabilities`
- `GET /api/v1/settings`
- `POST /api/v1/settings`
- `GET /api/v1/themes`
- `POST /api/v1/themes/active`
- `POST /api/v1/music/upload`
- `GET /api/v1/music/tracks`

Read-only endpoints added in `pi_runtime/outputs/shaer_pi_os/server.py`:

- `GET /api/v1/network/wifi`
- `GET /api/v1/network/wifi/scan`
- `GET /api/v1/bluetooth`
- `GET /api/v1/bluetooth/scan`

## Backend Modules Used

- `pi_runtime/outputs/shaer_pi_os/server.py`: authenticated companion route handling, fixed-command Wi-Fi/Bluetooth probes.
- Existing generated `shaer_companion` import: pairing, dashboard, settings, theme, music upload, and library calls. Source for this generated module is not present under `pi_runtime/outputs/shaer_pi_os`, so this pass did not modify its internals.

## Test Performed

- Added unit route coverage in `pi_runtime/outputs/shaer_backend/tests/test_device_routes.py`.
- Tests fake SHAeR-side command output for `ip`, `iw`, `nmcli`, and `bluetoothctl`.
- Tests verify that Wi-Fi and Bluetooth data comes from the SHAeR backend routes, not the desktop computer.

## Known Failure Modes

- If `nmcli` is absent on the Pi, Wi-Fi scan returns `supported: false` instead of fake networks.
- If `bluetoothctl` is absent on the Pi, Bluetooth routes return `supported: false`.
- Settings not currently present in the backend settings schema may be rejected or may not affect hardware until the generated `shaer_companion` settings implementation maps them to device services.
- MP3 upload verification currently depends on `/api/v1/music/tracks` reflecting the uploaded file after upload. A stronger checksum-specific upload response still needs support in the generated companion service.
- Credentials are stored through the browser local storage shim in this first Tauri foundation. Native keychain storage is required before production packaging.

## Restart Persistence

- Desktop reconnection logic attempts to reload the stored real-device credential and calls authenticated `GET /api/v1/dashboard`.
- Restart persistence was not physically tested in this environment.

## Physical Verification

No physical SHAeR device was available in this run. Nothing in this ledger is marked physically verified.
