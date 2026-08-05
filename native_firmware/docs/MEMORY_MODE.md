# SHAeR Memory Mode V1

Status: V1 freeze draft.

## Purpose

Memory Mode turns SHAeR from a music player into a personal audio archive. It lets voice recordings attach to albums, playlists, songs, and Spotify/local library items.

## User Model

Examples:

```text
Artist
  -> Pink Floyd
    -> Album
      -> Wish You Were Here
        -> Voice Memory
          -> "Remember when we listened to this..."
```

```text
Playlist
  -> Road Trip
    -> 3 Voice Memories
```

## What Memory Mode Is

Memory Mode is metadata plus audio files. It does not require new hardware.

The recorder stores WAV and MP3 files. SQLite stores links between recordings and music objects.

## Link Targets

A voice memory may link to:

- Track.
- Album.
- Artist.
- Playlist.
- Spotify URI.
- Local file path.
- Local library item ID.

## Required Metadata

| Field | Purpose |
|---|---|
| memory_id | Stable ID |
| file_path | WAV or MP3 location |
| title | User-visible label |
| transcript | Optional later |
| created_at | Archive sorting |
| duration_seconds | Playback UI |
| linked_entity_type | track, album, playlist, artist, spotify_uri, local_file |
| linked_entity_id | Local DB ID or Spotify URI |
| favourite | User marker |
| rating | Optional emotional/quality marker |
| notes | Text note from companion app |

## Device UI

Memory Mode must be accessible from:

- Home.
- Now Playing.
- Local Library track.
- Album view.
- Playlist view.

Required actions:

- Record new memory.
- Attach to current song.
- Attach to album.
- Attach to playlist.
- Play memory.
- Delete memory after confirmation.
- Export memories through companion app.

## Recording Behavior

- Default save: WAV plus optional MP3.
- If storage is low, warn before recording.
- If shutdown begins during recording, finalize the current file before Linux shutdown.
- Sound effects should be muted while recording.

## Emotional Design

Memory Mode should feel quieter and more intimate than the music browser. It is not a notes app. It is a shelf of personal audio traces.

## Database Ownership

SQLite is the source of truth on device. The companion app may sync or export memories. PostgreSQL may be used only by a future desktop/server companion target.

