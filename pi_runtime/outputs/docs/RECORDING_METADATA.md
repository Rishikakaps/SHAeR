# Recording Metadata

## Sidecar Schema

Each WAV has a JSON sidecar with schema version 1:

```json
{
  "schema_version": 1,
  "recording_uuid": "uuid",
  "timestamp": 1783956042,
  "duration_ms": 12034,
  "file_size": 1155328,
  "device_theme": "shaer_dark_archive",
  "title": null,
  "favorite": false,
  "playback_position_ms": 0,
  "sync_status": "local",
  "status": "complete",
  "notes": null,
  "archive_folder": null,
  "file_path": "/absolute/archive/path.wav",
  "sidecar_path": "/absolute/archive/path.json"
}
```

## Identity and Time

The UUID is the stable identity across rename, move, backup, and restore. SQLite uses an integer row ID only as a local API key. `timestamp` is Unix time; display date and title are derived at read time when no user title exists.

## Search

Recording search is separate from music search. The archive supports:

- Title, date, and notes text.
- Favorite filter.
- Year and month filters.
- Minimum and maximum duration.
- Recent ordering by capture time.

Notes are stored now so future annotation features do not require a destructive schema change.

## Atomicity

Sidecars are written to a temporary file in the destination directory, flushed with `fsync`, then atomically renamed. Database updates rewrite the sidecar so exported files retain current user metadata.
