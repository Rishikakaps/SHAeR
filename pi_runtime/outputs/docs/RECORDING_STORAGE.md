# Recording Storage

## Layout

```text
Recordings/
  2026/
    July/
      2026-07-13_18-10-42_ab12cd34.wav
      2026-07-13_18-10-42_ab12cd34.json
  Archive/
    Field_Notes/
      ...
```

Active capture uses two additional temporary files:

- `*.wav.partial`: audio being written.
- `*.recording.json`: crash-recovery journal.

The final WAV and JSON sidecar are permanent user data. Recording media is never stored inside a theme directory or an update payload.

## SQLite Index

The default database is `<config>/recordings.db`. It uses WAL mode and is kept separate from `library.db`. Indexed fields include title, timestamp, duration, favorite state, playback position, sync state, recovery state, notes, and archive folder.

## Free-Space Policy

Capture will not start below `SHAER_RECORDING_MIN_FREE_BYTES` (128 MiB by default). The monitor checks storage during capture and finalizes with reason `low_storage` before the filesystem is exhausted.

## Backups and Updates

Encrypted companion backups include:

- A consistent SQLite snapshot at `recordings/recordings.db`.
- Every WAV, sidecar, and retained recovery artifact under `recordings/files/`.

Restore stages the database as `recordings.db.restore-pending`; the Pi service applies it before opening SQLite on the next start. Theme and firmware updates must not delete the configured recording directory.

## Export and Delete

Authenticated companion downloads stream `audio/wav`. Delete removes both the WAV and sidecar after resolving both paths inside the configured archive root. Move and duplicate operations remain inside that root.
