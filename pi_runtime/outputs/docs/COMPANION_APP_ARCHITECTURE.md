# SHAeR Companion App Architecture

## Boundaries

The Raspberry Pi firmware remains the authority for playback, navigation, themes, the SQLite library, Spotify, diagnostics, settings, and hardware. The companion is a management client. Its playback buttons emit the same logical actions as the physical controls and do not create a second player.

## Components

- `shaer_companion/protocol.py`: transport-independent services, pairing, persistence, music, themes, diagnostics, updates, and backup.
- `shaer_pi_os/server.py`: Wi-Fi transport router and authentication boundary.
- `shaer_pi_os/hardware-bridge.js`: physical pairing confirmation and live theme-switch consumption.
- `shaer_companion/index.html`: responsive companion application.
- `shaer_companion/src/companion.js`: API client and view state.
- `shaer_companion/src/companion.css`: quiet operational interface for phone and desktop widths.
- `theme_validation/`: deterministic six-theme visual regression harness.

## Data flow

```text
Companion UI -> versioned JSON API -> CompanionService -> existing SHAeR services
                                      |-> SQLite music library
                                      |-> settings store
                                      |-> diagnostics scripts
                                      |-> theme packages
                                      |-> update staging/helper boundary

Physical OK/Back -> existing event channel -> pairing approval overlay
```

## Independence

SHAeR continues to boot and operate when the companion is absent. Pairing tokens can be forgotten without affecting playback. Companion failure cannot replace or block the encoder and buttons.

## Validation

Unit tests cover pairing, settings, music indexing/playlists, encrypted backup, and update verification. An HTTP integration test covers discovery through authenticated mirrored controls. The theme harness renders 72 canonical states across six themes. Physical Pi, BLE, phone, and OTA acceptance remain **PENDING** until hardware is reachable.

