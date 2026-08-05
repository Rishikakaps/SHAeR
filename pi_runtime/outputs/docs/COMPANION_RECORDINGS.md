# Companion Recordings

## User Capabilities

The Recordings tab can:

- Browse all, recent, favorite, and month-filtered recordings.
- Search recording titles, dates, and notes.
- Rename and favorite.
- Download/export the original WAV.
- Duplicate or move to an archive folder.
- Delete with confirmation.
- Include recordings in encrypted backup and restore.

Storage usage and synchronization state are returned with the archive listing.

## Authentication

All `/api/v1/recordings` routes require a trusted companion bearer token. Device-only capture and inline playback routes accept loopback requests only. Tokens and recording bytes are never written to logs.

## API

- `GET /api/v1/recordings`: list and filter.
- `GET /api/v1/recordings/{id}`: metadata.
- `GET /api/v1/recordings/{id}/download`: WAV export.
- `POST /api/v1/recordings/{id}`: rename, favorite, notes, duplicate, or move.
- `DELETE /api/v1/recordings/{id}`: remove WAV, sidecar, and row.

Device routes:

- `GET /api/recording/status`
- `GET /api/recording/library`
- `GET /api/recording/audio/{id}`
- `POST /api/recording/control`

## Conflict Policy

The Pi archive is authoritative for audio files. Metadata uses an explicit `sync_status` value (`local`, `pending`, `synced`, or `conflict`). Duplicate and move operations generate or preserve sidecars immediately. Future remote sync can build on these states without mixing recordings into the music index.
