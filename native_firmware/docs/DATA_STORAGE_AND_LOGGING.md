# SHAeR Data Storage And Logging V1

Status: V1 freeze draft.

## Database Choice

Device V1 uses SQLite.

PostgreSQL is allowed only for a future companion-server or desktop-sync target. The handheld must not depend on PostgreSQL for boot, playback, recording, or recovery.

## Database Files

```text
data/
  shaer.db
  shaer.db-wal
  shaer.db-shm
```

Use SQLite WAL mode so reads remain responsive while metadata is updated.

## Required Tables

Minimum V1 tables:

- tracks.
- albums.
- artists.
- playlists.
- playlist_tracks.
- playback_history.
- voice_memories.
- memory_links.
- spotify_tokens.
- cover_art_cache.
- settings.
- migrations.

## Track Metadata

Tracks must support:

- Title.
- Artist.
- Album.
- Duration.
- Format.
- Sample rate.
- Bit depth or bitrate.
- ReplayGain track gain.
- ReplayGain album gain.
- Play count.
- Last played.
- Favourite.
- Rating.
- Spotify URI.
- Cover art cache path.

## Settings Persistence

Persist:

- Active theme.
- Volume.
- Crossfade setting.
- ReplayGain setting.
- Quality mode.
- Battery saver preference.
- Bluetooth known devices.
- Spotify authorization state.
- Last local playback position where safe.

Settings should be stored in SQLite and mirrored to human-readable JSON/YAML for debug and manual repair.

## Logs Directory

All logs live in:

```text
logs/
  system.log
  spotify.log
  audio.log
  power.log
  bluetooth.log
  crash.log
```

## Logging Rules

- Rotate logs by size.
- Keep enough history for support without filling the SD card.
- Never log raw Spotify secrets.
- Never log full private file paths in exported public reports unless user chooses full export.
- Crash logs must include firmware version, theme ID, state, screen, power mode, and last recovery action.

## Export

Recovery mode and companion app must support:

- Export logs.
- Export settings.
- Export database backup.
- Export voice memories.
- Export cover cache optionally.

