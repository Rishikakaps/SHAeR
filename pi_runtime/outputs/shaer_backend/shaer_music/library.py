"""Local SQLite music library for SHAeR Layer 11.

This module implements the Doc 14 database schema and a small repository layer
that can run on macOS for diagnostics and on Raspberry Pi for the real library.
GStreamer metadata extraction is used when PyGObject is available; otherwise
the indexer falls back to filename-derived metadata so tests remain portable.
"""

from __future__ import annotations

import json
import os
import random
import sqlite3
import time
from dataclasses import asdict, dataclass
from enum import Enum
from pathlib import Path
from typing import Iterable, Iterator, Sequence


SUPPORTED_EXTENSIONS = {".mp3", ".flac", ".wav", ".aac", ".m4a", ".ogg"}
SCHEMA_VERSION = 2


SCHEMA = """
CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS tracks (
    id INTEGER PRIMARY KEY,
    filepath TEXT NOT NULL UNIQUE,
    title TEXT,
    artist TEXT,
    album TEXT,
    album_artist TEXT,
    track_number INTEGER,
    duration_s INTEGER,
    codec TEXT,
    sample_rate INTEGER,
    bit_depth INTEGER,
    file_mtime INTEGER,
    cover_art_path TEXT,
    play_count INTEGER DEFAULT 0,
    last_played_at INTEGER,
    is_favourite INTEGER DEFAULT 0,
    total_listened_s INTEGER DEFAULT 0,
    isrc TEXT,
    spotify_track_id TEXT
);

CREATE TABLE IF NOT EXISTS playlists (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    created_at INTEGER,
    spotify_playlist_id TEXT
);

CREATE TABLE IF NOT EXISTS playlist_tracks (
    playlist_id INTEGER REFERENCES playlists(id),
    track_id INTEGER REFERENCES tracks(id),
    position INTEGER,
    PRIMARY KEY (playlist_id, track_id)
);

CREATE TABLE IF NOT EXISTS listening_sessions (
    id INTEGER PRIMARY KEY,
    track_id INTEGER REFERENCES tracks(id),
    started_at INTEGER,
    duration_listened_s INTEGER,
    source TEXT
);

CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist);
CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album);
CREATE INDEX IF NOT EXISTS idx_tracks_favourite ON tracks(is_favourite);

CREATE TABLE IF NOT EXISTS spotify_cache (
    cache_key TEXT PRIMARY KEY,
    entity_type TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    artwork_path TEXT,
    updated_at INTEGER NOT NULL,
    expires_at INTEGER
);

CREATE TABLE IF NOT EXISTS playback_state (
    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
    source TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    updated_at INTEGER NOT NULL
);
"""


@dataclass(slots=True)
class TrackRecord:
    filepath: str
    title: str | None = None
    artist: str | None = None
    album: str | None = None
    album_artist: str | None = None
    track_number: int | None = None
    duration_s: int | None = None
    codec: str | None = None
    sample_rate: int | None = None
    bit_depth: int | None = None
    file_mtime: int | None = None
    cover_art_path: str | None = None
    isrc: str | None = None
    spotify_track_id: str | None = None


class LibraryDatabase:
    """Owns the SQLite connection and Doc 14 schema migrations."""

    def __init__(self, db_path: str | Path):
        self.path = Path(db_path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(self.path)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys = ON")
        self.connection.execute("PRAGMA journal_mode = WAL")
        self.connection.execute("PRAGMA synchronous = FULL")
        self.connection.execute("PRAGMA busy_timeout = 5000")
        self.connection.execute("PRAGMA wal_autocheckpoint = 1000")

    def migrate(self) -> None:
        with self.connection:
            self.connection.executescript(SCHEMA)
            existing_columns = {
                str(row["name"]) for row in self.connection.execute("PRAGMA table_info(tracks)")
            }
            additions = {
                "source": "TEXT NOT NULL DEFAULT 'local'",
                "availability": "TEXT NOT NULL DEFAULT 'local'",
                "spotify_uri": "TEXT",
                "spotify_artwork_url": "TEXT",
                "metadata_updated_at": "INTEGER",
            }
            for column, declaration in additions.items():
                if column not in existing_columns:
                    self.connection.execute(f"ALTER TABLE tracks ADD COLUMN {column} {declaration}")
            self.connection.execute(
                "INSERT OR IGNORE INTO schema_migrations(version, applied_at) VALUES (?, ?)",
                (SCHEMA_VERSION, int(time.time())),
            )

    def close(self) -> None:
        self.connection.close()


class TrackDatabase(LibraryDatabase):
    """Compatibility name for Layer 11's TrackDatabase checkpoint."""


class StatisticsDatabase(LibraryDatabase):
    """Compatibility name for Layer 11's StatisticsDatabase checkpoint."""


class TrackRepository:
    def __init__(self, db: LibraryDatabase):
        self.db = db

    def upsert(self, track: TrackRecord) -> int:
        payload = asdict(track)
        if payload["file_mtime"] is None and Path(track.filepath).exists():
            payload["file_mtime"] = int(Path(track.filepath).stat().st_mtime)
        columns = ", ".join(payload)
        placeholders = ", ".join(":" + key for key in payload)
        updates = ", ".join(f"{key}=excluded.{key}" for key in payload if key != "filepath")
        with self.db.connection:
            self.db.connection.execute(
                f"INSERT INTO tracks ({columns}) VALUES ({placeholders}) "
                f"ON CONFLICT(filepath) DO UPDATE SET {updates}",
                payload,
            )
            row = self.db.connection.execute(
                "SELECT id FROM tracks WHERE filepath = ?", (track.filepath,)
            ).fetchone()
        return int(row["id"])

    def get(self, track_id: int) -> sqlite3.Row | None:
        return self.db.connection.execute("SELECT * FROM tracks WHERE id = ?", (track_id,)).fetchone()

    def all(self) -> list[sqlite3.Row]:
        return list(self.db.connection.execute("SELECT * FROM tracks ORDER BY artist, album, track_number, title"))

    def artists(self) -> list[str]:
        rows = self.db.connection.execute(
            "SELECT DISTINCT artist FROM tracks WHERE artist IS NOT NULL ORDER BY artist"
        )
        return [str(row["artist"]) for row in rows]

    def albums(self) -> list[str]:
        rows = self.db.connection.execute(
            "SELECT DISTINCT album FROM tracks WHERE album IS NOT NULL ORDER BY album"
        )
        return [str(row["album"]) for row in rows]

    def favourites(self) -> list[sqlite3.Row]:
        return list(self.db.connection.execute("SELECT * FROM tracks WHERE is_favourite = 1 ORDER BY title"))

    def search(self, query: str) -> list[sqlite3.Row]:
        pattern = f"%{query.lower()}%"
        return list(
            self.db.connection.execute(
                """
                SELECT * FROM tracks
                WHERE lower(coalesce(title, '')) LIKE ?
                   OR lower(coalesce(artist, '')) LIKE ?
                   OR lower(coalesce(album, '')) LIKE ?
                ORDER BY artist, album, title
                """,
                (pattern, pattern, pattern),
            )
        )

    def set_favourite(self, track_id: int, favourite: bool) -> None:
        with self.db.connection:
            self.db.connection.execute(
                "UPDATE tracks SET is_favourite = ? WHERE id = ?", (1 if favourite else 0, track_id)
            )

    def merge_spotify_metadata(
        self,
        track_id: int,
        spotify_track_id: str,
        spotify_uri: str,
        artwork_url: str | None = None,
    ) -> None:
        with self.db.connection:
            self.db.connection.execute(
                """
                UPDATE tracks
                   SET spotify_track_id = ?, spotify_uri = ?, spotify_artwork_url = ?,
                       source = 'unified', availability = 'offline', metadata_updated_at = ?
                 WHERE id = ?
                """,
                (spotify_track_id, spotify_uri, artwork_url, int(time.time()), track_id),
            )

    def upsert_spotify_only(
        self,
        spotify_track_id: str,
        spotify_uri: str,
        title: str,
        artist: str,
        album: str | None,
        duration_ms: int | None,
        isrc: str | None = None,
        artwork_url: str | None = None,
    ) -> int:
        filepath = f"spotify://{spotify_track_id}"
        existing = self.db.connection.execute(
            "SELECT id FROM tracks WHERE spotify_track_id = ? OR filepath = ?",
            (spotify_track_id, filepath),
        ).fetchone()
        duration_s = int(duration_ms / 1000) if duration_ms is not None else None
        with self.db.connection:
            if existing:
                track_id = int(existing["id"])
                self.db.connection.execute(
                    """
                    UPDATE tracks SET title = ?, artist = ?, album = ?, duration_s = ?, isrc = ?,
                        spotify_track_id = ?, spotify_uri = ?, spotify_artwork_url = ?,
                        source = 'spotify', availability = 'spotify', metadata_updated_at = ?
                    WHERE id = ?
                    """,
                    (
                        title, artist, album, duration_s, isrc, spotify_track_id, spotify_uri,
                        artwork_url, int(time.time()), track_id,
                    ),
                )
                return track_id
            cursor = self.db.connection.execute(
                """
                INSERT INTO tracks(
                    filepath, title, artist, album, duration_s, isrc, spotify_track_id,
                    spotify_uri, spotify_artwork_url, source, availability, metadata_updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'spotify', 'spotify', ?)
                """,
                (
                    filepath, title, artist, album, duration_s, isrc, spotify_track_id,
                    spotify_uri, artwork_url, int(time.time()),
                ),
            )
            return int(cursor.lastrowid)


class PlaylistRepository:
    def __init__(self, db: LibraryDatabase):
        self.db = db

    def create(self, name: str, spotify_playlist_id: str | None = None) -> int:
        with self.db.connection:
            cursor = self.db.connection.execute(
                "INSERT INTO playlists(name, created_at, spotify_playlist_id) VALUES (?, ?, ?)",
                (name, int(time.time()), spotify_playlist_id),
            )
        return int(cursor.lastrowid)

    def add_track(self, playlist_id: int, track_id: int, position: int) -> None:
        with self.db.connection:
            self.db.connection.execute(
                """
                INSERT INTO playlist_tracks(playlist_id, track_id, position)
                VALUES (?, ?, ?)
                ON CONFLICT(playlist_id, track_id) DO UPDATE SET position = excluded.position
                """,
                (playlist_id, track_id, position),
            )

    def tracks(self, playlist_id: int) -> list[sqlite3.Row]:
        return list(
            self.db.connection.execute(
                """
                SELECT tracks.* FROM playlist_tracks
                JOIN tracks ON tracks.id = playlist_tracks.track_id
                WHERE playlist_tracks.playlist_id = ?
                ORDER BY playlist_tracks.position
                """,
                (playlist_id,),
            )
        )

    def all(self) -> list[sqlite3.Row]:
        return list(self.db.connection.execute("SELECT * FROM playlists ORDER BY name"))


class RepeatMode(str, Enum):
    OFF = "off"
    ONE = "one"
    ALL = "all"


@dataclass
class PlaybackQueue:
    track_ids: list[int]
    current_index: int = 0
    shuffle_enabled: bool = False
    repeat_mode: RepeatMode = RepeatMode.OFF
    _shuffle_order: list[int] | None = None

    def current(self) -> int | None:
        if not self.track_ids:
            return None
        if self.shuffle_enabled and self._shuffle_order is not None:
            return self.track_ids[self._shuffle_order[self.current_index]]
        return self.track_ids[self.current_index]

    def set_shuffle(self, enabled: bool) -> None:
        if enabled and not self.shuffle_enabled:
            self._shuffle_order = list(range(len(self.track_ids)))
            current = self.current_index
            random.shuffle(self._shuffle_order)
            if current in self._shuffle_order:
                self._shuffle_order.remove(current)
            self._shuffle_order.insert(0, current)
            self.current_index = 0
        if not enabled and self.shuffle_enabled:
            current_track = self.current()
            self._shuffle_order = None
            if current_track in self.track_ids:
                self.current_index = self.track_ids.index(current_track)
        self.shuffle_enabled = enabled

    def ordered_ids(self) -> list[int]:
        if self.shuffle_enabled and self._shuffle_order is not None:
            return [self.track_ids[index] for index in self._shuffle_order]
        return list(self.track_ids)

    def next_track_id(self) -> int | None:
        if not self.track_ids:
            return None
        order = self.ordered_ids()
        if self.repeat_mode == RepeatMode.ONE:
            return order[self.current_index]
        if self.current_index + 1 < len(order):
            self.current_index += 1
            return order[self.current_index]
        if self.repeat_mode == RepeatMode.ALL:
            self.current_index = 0
            return order[0]
        return None

    def previous_track_id(self) -> int | None:
        if not self.track_ids:
            return None
        order = self.ordered_ids()
        self.current_index = max(0, self.current_index - 1)
        return order[self.current_index]


class QueueRepository:
    """Persists the session queue snapshot as Doc 14 specifies."""

    def __init__(self, snapshot_path: str | Path):
        self.path = Path(snapshot_path)
        self.path.parent.mkdir(parents=True, exist_ok=True)

    def save(self, queue: PlaybackQueue) -> None:
        payload = {
            "track_ids": queue.track_ids,
            "current_index": queue.current_index,
            "shuffle_enabled": queue.shuffle_enabled,
            "repeat_mode": queue.repeat_mode.value,
            "shuffle_order": queue._shuffle_order,
        }
        self.path.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    def load(self) -> PlaybackQueue:
        if not self.path.exists():
            return PlaybackQueue([])
        payload = json.loads(self.path.read_text(encoding="utf-8"))
        queue = PlaybackQueue(
            track_ids=[int(value) for value in payload.get("track_ids", [])],
            current_index=int(payload.get("current_index", 0)),
            shuffle_enabled=bool(payload.get("shuffle_enabled", False)),
            repeat_mode=RepeatMode(payload.get("repeat_mode", RepeatMode.OFF.value)),
        )
        order = payload.get("shuffle_order")
        queue._shuffle_order = [int(value) for value in order] if order is not None else None
        return queue


class SessionRepository:
    def __init__(self, db: LibraryDatabase):
        self.db = db

    def record_listen(self, track_id: int, duration_listened_s: int, source: str = "local") -> None:
        started_at = int(time.time()) - max(0, int(duration_listened_s))
        with self.db.connection:
            self.db.connection.execute(
                """
                INSERT INTO listening_sessions(track_id, started_at, duration_listened_s, source)
                VALUES (?, ?, ?, ?)
                """,
                (track_id, started_at, int(duration_listened_s), source),
            )
            self.db.connection.execute(
                """
                UPDATE tracks
                   SET play_count = play_count + 1,
                       last_played_at = ?,
                       total_listened_s = total_listened_s + ?
                 WHERE id = ?
                """,
                (int(time.time()), int(duration_listened_s), track_id),
            )

    def history(self) -> list[sqlite3.Row]:
        return list(
            self.db.connection.execute(
                """
                SELECT listening_sessions.*, tracks.title, tracks.artist
                FROM listening_sessions
                JOIN tracks ON tracks.id = listening_sessions.track_id
                ORDER BY started_at DESC
                """
            )
        )


class LibraryIndexer:
    def __init__(self, db: LibraryDatabase, music_root: str | Path, artwork_cache: str | Path | None = None):
        self.db = db
        self.music_root = Path(music_root)
        self.artwork_cache = Path(artwork_cache) if artwork_cache else self.music_root / ".shaer_artwork"
        self.tracks = TrackRepository(db)

    def scan_files(self) -> Iterator[Path]:
        if not self.music_root.exists():
            return iter(())
        return (
            path
            for path in self.music_root.rglob("*")
            if path.is_file() and path.suffix.lower() in SUPPORTED_EXTENSIONS
        )

    def index(self) -> dict[str, int]:
        report = {"scanned": 0, "indexed": 0, "invalid": 0, "unchanged": 0}
        for path in self.scan_files():
            report["scanned"] += 1
            try:
                stat = path.stat()
            except OSError:
                report["invalid"] += 1
                continue
            existing = self.db.connection.execute(
                "SELECT file_mtime FROM tracks WHERE filepath = ?", (str(path),)
            ).fetchone()
            if existing and int(existing["file_mtime"] or 0) == int(stat.st_mtime):
                report["unchanged"] += 1
                continue
            self.tracks.upsert(self.extract_metadata(path, int(stat.st_mtime)))
            report["indexed"] += 1
        return report

    def extract_metadata(self, path: Path, file_mtime: int | None = None) -> TrackRecord:
        metadata = self._gstreamer_metadata(path)
        if metadata is None:
            metadata = {
                "title": path.stem,
                "artist": "Unknown Artist",
                "album": "Unknown Album",
                "codec": path.suffix.lower().lstrip(".").upper(),
            }
        return TrackRecord(filepath=str(path), file_mtime=file_mtime, **metadata)

    def _gstreamer_metadata(self, path: Path) -> dict[str, object] | None:
        try:
            import gi  # type: ignore

            gi.require_version("Gst", "1.0")
            gi.require_version("GstPbutils", "1.0")
            from gi.repository import Gst, GstPbutils  # type: ignore
        except Exception:
            return None
        Gst.init(None)
        try:
            discoverer = GstPbutils.Discoverer.new(10 * Gst.SECOND)
            info = discoverer.discover_uri(path.resolve().as_uri())
            tags = info.get_tags()
        except Exception:
            return None

        def tag_string(name: str) -> str | None:
            if not tags:
                return None
            ok, value = tags.get_string(name)
            return value if ok else None

        audio_streams = info.get_audio_streams()
        stream = audio_streams[0] if audio_streams else None
        duration = info.get_duration()
        return {
            "title": tag_string("title") or path.stem,
            "artist": tag_string("artist") or "Unknown Artist",
            "album": tag_string("album") or "Unknown Album",
            "album_artist": tag_string("album-artist"),
            "duration_s": int(duration / Gst.SECOND) if duration else None,
            "codec": stream.get_codec() if stream else path.suffix.lower().lstrip(".").upper(),
            "sample_rate": stream.get_sample_rate() if stream else None,
            "bit_depth": stream.get_depth() if stream else None,
            "isrc": tag_string("isrc"),
        }
