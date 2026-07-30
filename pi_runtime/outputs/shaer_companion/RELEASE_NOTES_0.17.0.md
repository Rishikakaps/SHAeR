# SHAeR Companion 0.17.0

## Included

- One shared application for Android, Windows PWA, and browser fallback.
- Local-network discovery with manual address fallback.
- Physical-approval pairing, remembered devices, linked-device listing, and revocation.
- Real SHAeR dashboard, playback, local library, playlists, recordings, themes, settings, diagnostics, updates, and backup/restore.
- Real Spotify status, current track, artwork, queue, saved tracks, playlists, recent tracks, search, playback, and supported volume control.
- Explicit connecting, offline, unauthenticated, loading, empty, and error states without demo-data substitution.
- Android Keystore credential encryption and encrypted browser IndexedDB storage with session-only fallback.
- Responsive phone and desktop layout with keyboard and selected-tab semantics.

## Artifacts

- `SHAeR-Companion-0.17.0-debug.apk`
- `shaer-companion-pwa-0.17.0.zip`
- matching `.sha256` checksum files

## Compatibility

- SHAeR companion protocol: v1
- SHAeR Raspberry Pi prototype firmware: 0.16.x
- Android: API 24 or newer
- Windows: current Microsoft Edge with installable PWA support

## Not Yet A Consumer Release

- The APK is debug-signed and still requires physical Android acceptance.
- Windows installation and uninstall behavior still require a real Windows test.
- Physical SHAeR discovery, OK-button pairing, real Spotify playback, reboot reconnection, and revocation still require end-to-end hardware acceptance.
- Local access is authenticated but currently uses HTTP. TLS or another pinned encrypted transport remains an open release gate.
