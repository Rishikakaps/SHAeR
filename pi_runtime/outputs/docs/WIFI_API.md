# SHAeR Wi-Fi API

The device advertises `_shaer._tcp` through Avahi and is normally reachable as `http://shaer.local:8775/companion`. Manual host entry remains available when mDNS is blocked.

## Public routes

- `GET /api/v1/device/discovery`
- `POST /api/v1/pairing/start`
- `GET /api/v1/pairing/status?pairing_id=...`

`POST /api/v1/pairing/approve` and `GET /api/v1/pairing/pending` are loopback-only because they belong to the physical SHAeR UI. Approval also consumes a short-lived, one-use nonce issued by the physical OK button.

## Authenticated routes

- `GET /api/v1/dashboard`
- `POST /api/v1/playback/control`
- `GET|POST /api/v1/settings`
- `GET /api/v1/themes`
- `POST /api/v1/themes/active`
- `GET /api/v1/themes/{id}/export`
- `DELETE /api/v1/themes/{id}`
- `GET /api/v1/music/tracks?q=`
- `POST /api/v1/music/upload`
- `POST /api/v1/music/tracks/{id}`
- `DELETE /api/v1/music/tracks/{id}`
- `GET|POST /api/v1/music/playlists`
- `GET|POST|DELETE /api/v1/music/playlists/{id}`
- `GET /api/v1/music/playlists/{id}/export`
- `POST /api/v1/music/playlists/import`
- `GET /api/v1/diagnostics`
- `POST /api/v1/diagnostics/run`
- `GET /api/v1/updates/status`
- `POST /api/v1/updates/stage|install|rollback`
- `POST /api/v1/backup/create|restore`

Unsigned theme import is disabled. Built-in themes ship with the signed device bundle and can still be selected or exported.

File payloads are base64 in JSON and limited to 64 MB each. Supported audio formats are MP3, FLAC, WAV, AAC, OGG, and M4A. The file signature must match its extension. Successful upload triggers the existing SQLite indexer immediately.

Wi-Fi acceptance on the physical Raspberry Pi is **PENDING** until the device is reachable for a real phone test.
