from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Iterable

from .models import PlaylistMapping, TrackMetadata


class ClosingConnection(sqlite3.Connection):
    def __exit__(self, exc_type, exc_value, traceback) -> bool:
        result = super().__exit__(exc_type, exc_value, traceback)
        self.close()
        return result


class DatabaseManager:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.root.mkdir(parents=True, exist_ok=True)
        self.library_db = self.root / "library.db"
        self.playlists_db = self.root / "playlists.db"
        self.artwork_db = self.root / "artwork_cache.db"
        self.settings_db = self.root / "settings.db"
        self.legacy_settings_db = self.root / "device_settings.db"
        self.firmware_db = self.root / "firmware_versions.db"
        self.sync_db = self.root / "sync_history.db"
        self.voice_notes_db = self.root / "voice_notes.db"
        self.theme_db = self.root / "theme.db"

    def connect(self, path: Path) -> sqlite3.Connection:
        conn = sqlite3.connect(path, factory=ClosingConnection)
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA foreign_keys=ON")
        conn.execute("PRAGMA journal_mode=WAL")
        return conn

    def migrate(self) -> None:
        with self.connect(self.library_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS tracks (
                    content_hash TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL,
                    genre TEXT,
                    year TEXT,
                    track_number TEXT,
                    duration_seconds REAL,
                    source_path TEXT NOT NULL,
                    file_format TEXT NOT NULL,
                    file_size INTEGER NOT NULL,
                    artwork_path TEXT,
                    spotify_uri TEXT,
                    album_art_url TEXT,
                    popularity INTEGER,
                    provider TEXT NOT NULL,
                    favorite INTEGER NOT NULL DEFAULT 0,
                    play_count INTEGER NOT NULL DEFAULT 0,
                    last_played TEXT,
                    imported_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist);
                CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album);
                CREATE INDEX IF NOT EXISTS idx_tracks_genre ON tracks(genre);
                CREATE INDEX IF NOT EXISTS idx_tracks_year ON tracks(year);
                CREATE INDEX IF NOT EXISTS idx_tracks_recent ON tracks(imported_at);
                CREATE VIRTUAL TABLE IF NOT EXISTS track_search USING fts5(
                    title, artist, album, genre, content_hash UNINDEXED
                );
                CREATE TRIGGER IF NOT EXISTS tracks_ai AFTER INSERT ON tracks BEGIN
                    INSERT INTO track_search(title, artist, album, genre, content_hash)
                    VALUES (new.title, new.artist, new.album, new.genre, new.content_hash);
                END;
                CREATE TRIGGER IF NOT EXISTS tracks_ad AFTER DELETE ON tracks BEGIN
                    DELETE FROM track_search WHERE content_hash = old.content_hash;
                END;
                CREATE TRIGGER IF NOT EXISTS tracks_au AFTER UPDATE ON tracks BEGIN
                    DELETE FROM track_search WHERE content_hash = old.content_hash;
                    INSERT INTO track_search(title, artist, album, genre, content_hash)
                    VALUES (new.title, new.artist, new.album, new.genre, new.content_hash);
                END;
                """
            )
        with self.connect(self.playlists_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS playlists (
                    id INTEGER PRIMARY KEY,
                    name TEXT NOT NULL UNIQUE,
                    artwork_path TEXT,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE IF NOT EXISTS playlist_tracks (
                    playlist_id INTEGER NOT NULL,
                    track_hash TEXT NOT NULL,
                    position INTEGER NOT NULL,
                    PRIMARY KEY (playlist_id, position),
                    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE
                );
                CREATE INDEX IF NOT EXISTS idx_playlist_tracks_hash ON playlist_tracks(track_hash);
                """
            )
        with self.connect(self.settings_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS settings (
                    key TEXT PRIMARY KEY,
                    value TEXT NOT NULL,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE IF NOT EXISTS devices (
                    id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    mount_path TEXT,
                    firmware_version TEXT,
                    last_seen TEXT
                );
                CREATE TABLE IF NOT EXISTS bluetooth_devices (
                    address TEXT PRIMARY KEY,
                    name TEXT,
                    last_connected TEXT
                );
                CREATE TABLE IF NOT EXISTS wifi_profiles (
                    ssid TEXT PRIMARY KEY,
                    security TEXT,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
        if self.legacy_settings_db.exists() and not self.settings_db.exists():
            self.legacy_settings_db.replace(self.settings_db)
        with self.connect(self.artwork_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS artwork (
                    cache_key TEXT PRIMARY KEY,
                    source TEXT NOT NULL,
                    local_path TEXT NOT NULL,
                    width INTEGER,
                    height INTEGER,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
        with self.connect(self.firmware_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS firmware_versions (
                    version TEXT PRIMARY KEY,
                    path TEXT NOT NULL,
                    sha256 TEXT NOT NULL,
                    compatible_from TEXT,
                    added_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
        with self.connect(self.sync_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS sync_runs (
                    id INTEGER PRIMARY KEY,
                    device_path TEXT NOT NULL,
                    copied_files INTEGER NOT NULL,
                    copied_bytes INTEGER NOT NULL,
                    status TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )
        with self.connect(self.voice_notes_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS voice_notes (
                    id INTEGER PRIMARY KEY,
                    title TEXT NOT NULL,
                    audio_path TEXT NOT NULL,
                    audio_format TEXT NOT NULL DEFAULT 'mp3',
                    linked_type TEXT NOT NULL,
                    linked_id TEXT NOT NULL,
                    duration_seconds REAL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE INDEX IF NOT EXISTS idx_voice_notes_link ON voice_notes(linked_type, linked_id);
                """
            )
        with self.connect(self.theme_db) as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS themes (
                    id TEXT PRIMARY KEY,
                    display_name TEXT NOT NULL,
                    manifest_path TEXT NOT NULL,
                    active INTEGER NOT NULL DEFAULT 0,
                    installed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                CREATE TABLE IF NOT EXISTS theme_customizations (
                    theme_id TEXT NOT NULL,
                    key TEXT NOT NULL,
                    value TEXT NOT NULL,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    PRIMARY KEY(theme_id, key)
                );
                """
            )

    def upsert_tracks(self, tracks: Iterable[TrackMetadata]) -> int:
        rows = [
            (
                t.content_hash,
                t.title,
                t.artist,
                t.album,
                t.genre,
                t.year,
                t.track_number,
                t.duration_seconds,
                str(t.source_path),
                t.file_format,
                t.file_size,
                str(t.artwork_path) if t.artwork_path else None,
                t.spotify_uri,
                t.album_art_url,
                t.popularity,
                t.provider,
            )
            for t in tracks
        ]
        with self.connect(self.library_db) as conn:
            conn.executemany(
                """
                INSERT INTO tracks (
                    content_hash, title, artist, album, genre, year, track_number,
                    duration_seconds, source_path, file_format, file_size, artwork_path,
                    spotify_uri, album_art_url, popularity, provider
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(content_hash) DO UPDATE SET
                    title=excluded.title,
                    artist=excluded.artist,
                    album=excluded.album,
                    genre=excluded.genre,
                    year=excluded.year,
                    track_number=excluded.track_number,
                    duration_seconds=excluded.duration_seconds,
                    source_path=excluded.source_path,
                    file_format=excluded.file_format,
                    file_size=excluded.file_size,
                    artwork_path=excluded.artwork_path,
                    spotify_uri=excluded.spotify_uri,
                    album_art_url=excluded.album_art_url,
                    popularity=excluded.popularity,
                    provider=excluded.provider
                """,
                rows,
            )
        return len(rows)

    def upsert_playlists(self, playlists: Iterable[PlaylistMapping]) -> int:
        count = 0
        with self.connect(self.playlists_db) as conn:
            for playlist in playlists:
                cursor = conn.execute(
                    "INSERT INTO playlists(name, artwork_path) VALUES (?, ?) "
                    "ON CONFLICT(name) DO UPDATE SET artwork_path=excluded.artwork_path RETURNING id",
                    (playlist.name, str(playlist.artwork_path) if playlist.artwork_path else None),
                )
                playlist_id = int(cursor.fetchone()[0])
                conn.execute("DELETE FROM playlist_tracks WHERE playlist_id = ?", (playlist_id,))
                conn.executemany(
                    "INSERT INTO playlist_tracks(playlist_id, track_hash, position) VALUES (?, ?, ?)",
                    [(playlist_id, h, index) for index, h in enumerate(playlist.track_hashes)],
                )
                count += 1
        return count

    def tracks(self) -> list[sqlite3.Row]:
        with self.connect(self.library_db) as conn:
            return list(conn.execute("SELECT * FROM tracks ORDER BY artist, album, track_number, title"))

    def track_by_hash(self, content_hash: str) -> sqlite3.Row | None:
        with self.connect(self.library_db) as conn:
            return conn.execute("SELECT * FROM tracks WHERE content_hash = ?", (content_hash,)).fetchone()

    def duplicates(self) -> list[sqlite3.Row]:
        with self.connect(self.library_db) as conn:
            return list(
                conn.execute(
                    """
                    SELECT title, artist, album, file_size, COUNT(*) AS copies,
                           GROUP_CONCAT(source_path, '\n') AS paths
                    FROM tracks
                    GROUP BY lower(title), lower(artist), duration_seconds, file_size
                    HAVING COUNT(*) > 1
                    ORDER BY copies DESC, artist, title
                    """
                )
            )

    def library_stats(self) -> dict:
        with self.connect(self.library_db) as conn:
            row = conn.execute(
                """
                SELECT COUNT(*) AS tracks,
                       COUNT(DISTINCT artist) AS artists,
                       COUNT(DISTINCT album) AS albums,
                       COALESCE(SUM(file_size), 0) AS bytes
                FROM tracks
                """
            ).fetchone()
        with self.connect(self.playlists_db) as conn:
            playlists = conn.execute("SELECT COUNT(*) FROM playlists").fetchone()[0]
        return {
            "tracks": int(row["tracks"]),
            "artists": int(row["artists"]),
            "albums": int(row["albums"]),
            "playlists": int(playlists),
            "bytes": int(row["bytes"]),
        }

    def playlists(self) -> list[sqlite3.Row]:
        with self.connect(self.playlists_db) as conn:
            return list(
                conn.execute(
                    """
                    SELECT p.id, p.name, p.artwork_path, COUNT(pt.track_hash) AS track_count
                    FROM playlists p
                    LEFT JOIN playlist_tracks pt ON pt.playlist_id = p.id
                    GROUP BY p.id
                    ORDER BY p.name
                    """
                )
            )

    def upsert_playlist(self, name: str, track_hashes: list[str]) -> int:
        with self.connect(self.playlists_db) as conn:
            cursor = conn.execute(
                "INSERT INTO playlists(name) VALUES (?) "
                "ON CONFLICT(name) DO UPDATE SET name=excluded.name RETURNING id",
                (name,),
            )
            playlist_id = int(cursor.fetchone()[0])
            conn.execute("DELETE FROM playlist_tracks WHERE playlist_id = ?", (playlist_id,))
            conn.executemany(
                "INSERT INTO playlist_tracks(playlist_id, track_hash, position) VALUES (?, ?, ?)",
                [(playlist_id, track_hash, index) for index, track_hash in enumerate(track_hashes)],
            )
            return playlist_id

    def set_setting(self, key: str, value: str) -> None:
        with self.connect(self.settings_db) as conn:
            conn.execute(
                "INSERT INTO settings(key, value) VALUES (?, ?) "
                "ON CONFLICT(key) DO UPDATE SET value=excluded.value, updated_at=CURRENT_TIMESTAMP",
                (key, value),
            )

    def settings(self) -> dict[str, str]:
        with self.connect(self.settings_db) as conn:
            rows = conn.execute("SELECT key, value FROM settings ORDER BY key").fetchall()
        return {str(row["key"]): str(row["value"]) for row in rows}

    def add_voice_note(
        self,
        title: str,
        audio_path: Path,
        linked_type: str,
        linked_id: str,
        audio_format: str = "mp3",
        duration_seconds: float | None = None,
    ) -> int:
        with self.connect(self.voice_notes_db) as conn:
            cursor = conn.execute(
                """
                INSERT INTO voice_notes(title, audio_path, audio_format, linked_type, linked_id, duration_seconds)
                VALUES (?, ?, ?, ?, ?, ?)
                """,
                (title, str(audio_path), audio_format, linked_type, linked_id, duration_seconds),
            )
            return int(cursor.lastrowid)

    def voice_notes(self) -> list[sqlite3.Row]:
        with self.connect(self.voice_notes_db) as conn:
            return list(conn.execute("SELECT * FROM voice_notes ORDER BY created_at DESC, id DESC"))

    def register_theme(self, theme_id: str, display_name: str, manifest_path: Path, active: bool = False) -> None:
        with self.connect(self.theme_db) as conn:
            if active:
                conn.execute("UPDATE themes SET active = 0")
            conn.execute(
                "INSERT INTO themes(id, display_name, manifest_path, active) VALUES (?, ?, ?, ?) "
                "ON CONFLICT(id) DO UPDATE SET display_name=excluded.display_name, manifest_path=excluded.manifest_path, active=excluded.active",
                (theme_id, display_name, str(manifest_path), 1 if active else 0),
            )

    def themes(self) -> list[sqlite3.Row]:
        with self.connect(self.theme_db) as conn:
            return list(conn.execute("SELECT * FROM themes ORDER BY display_name"))
