# SHAeR Desktop Companion

This is the narrow vertical-slice desktop companion. It intentionally does not implement every planned SHAeR desktop page yet.

Implemented focus:

- Real manual connection to an existing SHAeR backend
- Existing pairing flow
- Reconnection using a persisted credential
- Real dashboard/status reads
- Five settings controls with read-back checks
- One MP3 upload flow with library read-back verification
- Real SHAeR-originated Wi-Fi and Bluetooth status/scan reads
- Development-only mock mode with a permanent visible banner

## Run

From this directory:

```bash
npm install
npm run tauri:dev
```

For browser-only development of the frontend:

```bash
npm install
npm run dev
```

## Build

Tauri packaging requires Rust/Cargo on the machine PATH. Install it first if
`cargo --version` does not print a version.

```bash
npm run build
npm run tauri:build
```

The end-user installer path depends on the host platform and Tauri bundler output under `src-tauri/target/release/bundle`.
On macOS, `npm run tauri:build:app` creates the `.app` bundle without also creating a DMG.

## Important Limits

- Do not call mock mode a connected SHAeR session.
- Do not mark settings as complete unless the backend accepts the update and the desktop reads the same value back.
- Do not mark MP3 transfer as complete after upload alone; the track must appear in `/api/v1/music/tracks`.
- Wi-Fi connect and Bluetooth pair/connect are intentionally not exposed yet because safe backend write operations were not present in this pass.
- Credentials currently use local storage. Replace with native secure credential storage before production installers.
