# SHAeR Companion Product Status

Date: 2026-07-18
Version: 0.17.0

## Implemented And Host-Validated

- Shared browser/Android/Windows-PWA application code.
- Real device dashboard, local library, playlists, recordings, themes, settings, diagnostics, updates, and backup UI.
- Real Spotify playback, queue, saved tracks, playlists, recently played, search, artwork, URI playback, context playback, and supported volume controls.
- Explicit loading, real data, empty, error, offline, paired-disconnected, and unauthenticated states.
- Android NSD local discovery and manual-address fallback.
- Physical-confirmation pairing flow and linked-device revocation.
- Future-compatible `owner` role and permission fields.
- Android Keystore credential encryption.
- Browser encrypted IndexedDB vault with session-only fallback.
- Installable debug APK and versioned PWA zip with SHA-256 checksums.

## Host Validation Record

- Shared companion JavaScript contracts: 9 passed.
- Backend Spotify contracts: 28 passed.
- Companion protocol, loopback HTTP, and security integration: 18 passed.
- Runtime diagnostics: 35 passed.
- Six-theme state and render harness: 78 states, 0 errors, 0 warnings, 0 baseline mismatches.
- Responsive browser inspection at 390x844 and 1440x900: no clipped controls, horizontal overflow, broken images, or console warnings.
- Android package: application ID `in.shaer.companion`, version 0.17.0, min SDK 24, target SDK 36, APK Signature Scheme v2 verified.

Release checksums:

```text
34ca583b4fec41eb1c6d9aacd19f2750d8770f6f99430db31c9dc61639adadef  SHAeR-Companion-0.17.0-debug.apk
7fa920264581a004d17866ca45be613ae07fbea5d2f3ea416ad2b8fe0d4c5638  shaer-companion-pwa-0.17.0.zip
```

## Android Acceptance

| Requirement | Status |
| --- | --- |
| APK compiles and has a valid debug signature | PASS on macOS build host |
| APK installs on a real phone/tablet | PENDING PHYSICAL TEST |
| Android NSD discovers physical SHAeR | PENDING PHYSICAL TEST |
| Pairing code and physical OK approval | PENDING PHYSICAL TEST |
| Reconnection after SHAeR reboot | PENDING PHYSICAL TEST |
| Real Spotify/device data and controls | PENDING PHYSICAL TEST |
| Android lifecycle/background recovery | PENDING PHYSICAL TEST |

## Windows Acceptance

| Requirement | Status |
| --- | --- |
| Versioned installable PWA package builds | PASS on macOS build host |
| Installs through Edge on a Windows laptop | PENDING WINDOWS TEST |
| Discovery/manual connect to physical SHAeR | PENDING WINDOWS TEST |
| Pairing, real data, controls, restart/reconnect | PENDING WINDOWS TEST |
| Uninstall leaves no unnecessary credential | PENDING WINDOWS TEST |

## Open Product Gates

- Local device transport is authenticated but not yet TLS encrypted.
- Release APK signing and key custody are not configured.
- Windows Edge installation and notifications are untested.
- Android notification/file-transfer lifecycle polish is untested.
- No physical phone, Windows laptop, or SHAeR hardware acceptance has been claimed.
- Recorder transfer is implemented in the shared UI but still depends on physical recorder validation.

The companion must not be described as a finished consumer release until every pending physical acceptance row passes.
