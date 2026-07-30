"""SQLite archive entries, playback sources, and handwritten Marginalia pages."""

from __future__ import annotations

import os
import sqlite3
import tempfile
import time
from pathlib import Path
from typing import Any, Mapping


SCHEMA = """
CREATE TABLE IF NOT EXISTS archive_entries (
    id INTEGER PRIMARY KEY,
    title TEXT NOT NULL,
    artist TEXT NOT NULL DEFAULT '',
    album TEXT NOT NULL DEFAULT '',
    duration_ms INTEGER NOT NULL DEFAULT 0,
    cached_artwork_path TEXT,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS track_sources (
    id INTEGER PRIMARY KEY,
    archive_entry_id INTEGER NOT NULL REFERENCES archive_entries(id) ON DELETE CASCADE,
    source_type TEXT NOT NULL,
    source_identifier TEXT NOT NULL,
    local_path TEXT,
    created_at INTEGER NOT NULL,
    UNIQUE(source_type, source_identifier)
);
CREATE TABLE IF NOT EXISTS marginalia (
    id INTEGER PRIMARY KEY,
    archive_entry_id INTEGER NOT NULL REFERENCES archive_entries(id) ON DELETE CASCADE,
    image_path TEXT NOT NULL UNIQUE,
    created_at INTEGER NOT NULL,
    playback_position_ms INTEGER NOT NULL DEFAULT 0,
    theme_id TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_archive_entries_title ON archive_entries(title, artist, album);
CREATE INDEX IF NOT EXISTS idx_marginalia_entry ON marginalia(archive_entry_id, created_at);
"""


class ArchiveError(RuntimeError):
    def __init__(self, code: str, message: str, status: int = 400):
        super().__init__(message)
        self.code = code
        self.status = status


class MusicalArchive:
    def __init__(self, database_path: str | Path, marginalia_root: str | Path):
        self.database_path = Path(database_path).expanduser()
        self.root = Path(marginalia_root).expanduser()
        self.database_path.parent.mkdir(parents=True, exist_ok=True)
        self.root.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(self.database_path, check_same_thread=False)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys = ON")
        self.connection.execute("PRAGMA journal_mode = WAL")
        self.connection.execute("PRAGMA synchronous = FULL")
        self.connection.execute("PRAGMA busy_timeout = 5000")
        self.migrate()

    def migrate(self) -> None:
        with self.connection:
            self.connection.executescript(SCHEMA)

    def close(self) -> None:
        self.connection.close()

    @staticmethod
    def _within(path: Path, root: Path) -> bool:
        try:
            path.resolve().relative_to(root.resolve())
            return True
        except ValueError:
            return False

    @staticmethod
    def _track_values(track: Mapping[str, Any]) -> dict[str, Any]:
        return {
            "title": str(track.get("title") or "Unknown track").strip()[:240],
            "artist": str(track.get("artist") or "").strip()[:240],
            "album": str(track.get("album") or "").strip()[:240],
            "duration_ms": max(0, int(track.get("duration_ms") or (int(track.get("duration_s") or 0) * 1000))),
            "cached_artwork_path": track.get("cover_art_path") or track.get("cover_art") or track.get("spotify_artwork_url"),
        }

    @staticmethod
    def source_for(track: Mapping[str, Any]) -> tuple[str, str, str | None]:
        uri = str(track.get("uri") or track.get("spotify_uri") or "").strip()
        filepath = str(track.get("filepath") or track.get("local_path") or "").strip()
        if uri.startswith("spotify:") or track.get("spotify_track_id"):
            identifier = uri or f"spotify:track:{track['spotify_track_id']}"
            return "spotify", identifier, None
        identifier = filepath or str(track.get("id") or track.get("media_id") or "")
        return "local", identifier, filepath or None

    def create_or_get(self, track: Mapping[str, Any]) -> dict[str, Any]:
        values = self._track_values(track)
        source_type, identifier, local_path = self.source_for(track)
        if not identifier:
            raise ArchiveError("archive_source_required", "A stable playback source is required.")
        existing = self.connection.execute(
            "SELECT archive_entry_id FROM track_sources WHERE source_type = ? AND source_identifier = ?",
            (source_type, identifier),
        ).fetchone()
        now = int(time.time())
        with self.connection:
            if existing:
                entry_id = int(existing["archive_entry_id"])
                self.connection.execute(
                    "UPDATE archive_entries SET title=?, artist=?, album=?, duration_ms=?, cached_artwork_path=coalesce(?, cached_artwork_path), updated_at=? WHERE id=?",
                    (*values.values(), now, entry_id),
                )
            else:
                cursor = self.connection.execute(
                    "INSERT INTO archive_entries(title, artist, album, duration_ms, cached_artwork_path, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
                    (values["title"], values["artist"], values["album"], values["duration_ms"], values["cached_artwork_path"], now, now),
                )
                entry_id = int(cursor.lastrowid)
                self.connection.execute(
                    "INSERT INTO track_sources(archive_entry_id, source_type, source_identifier, local_path, created_at) VALUES (?, ?, ?, ?, ?)",
                    (entry_id, source_type, identifier, local_path, now),
                )
        return self.get(entry_id)

    def get(self, entry_id: int) -> dict[str, Any]:
        row = self.connection.execute("SELECT * FROM archive_entries WHERE id = ?", (int(entry_id),)).fetchone()
        if not row:
            raise ArchiveError("archive_entry_not_found", "Archive entry does not exist.", 404)
        return self._public(row)

    def list(self, query: str = "", limit: int = 200) -> list[dict[str, Any]]:
        values: list[Any] = []
        clause = ""
        if query.strip():
            pattern = f"%{query.strip().lower()}%"
            clause = "WHERE lower(title) LIKE ? OR lower(artist) LIKE ? OR lower(album) LIKE ?"
            values.extend((pattern, pattern, pattern))
        values.append(min(1000, max(1, int(limit))))
        rows = self.connection.execute(f"SELECT * FROM archive_entries {clause} ORDER BY updated_at DESC LIMIT ?", values)
        return [self._public(row) for row in rows]

    def add_source(self, entry_id: int, track: Mapping[str, Any], allow_ambiguous: bool = False) -> dict[str, Any]:
        entry = self.get(entry_id)
        source_type, identifier, local_path = self.source_for(track)
        if not identifier:
            raise ArchiveError("archive_source_required", "A stable playback source is required.")
        if source_type == "local":
            score = self.match_score(entry, track)
            if score < 70:
                raise ArchiveError("archive_match_too_weak", "The local track is not a conservative match.", 409)
            if score < 90 and not allow_ambiguous:
                raise ArchiveError("archive_match_confirmation_required", f"Possible match found ({score}/100).", 409)
        with self.connection:
            self.connection.execute(
                "INSERT OR IGNORE INTO track_sources(archive_entry_id, source_type, source_identifier, local_path, created_at) VALUES (?, ?, ?, ?, ?)",
                (int(entry_id), source_type, identifier, local_path, int(time.time())),
            )
        return self.get(entry_id)

    @staticmethod
    def match_score(entry: Mapping[str, Any], track: Mapping[str, Any]) -> int:
        def same(left: Any, right: Any) -> bool:
            return str(left or "").strip().casefold() == str(right or "").strip().casefold() and bool(str(left or "").strip())
        score = 40 if same(entry.get("title"), track.get("title")) else 0
        score += 35 if same(entry.get("artist"), track.get("artist")) else 0
        score += 15 if same(entry.get("album"), track.get("album")) else 0
        entry_duration = int(entry.get("duration_ms") or 0)
        track_duration = int(track.get("duration_ms") or (int(track.get("duration_s") or 0) * 1000))
        if entry_duration and track_duration and abs(entry_duration - track_duration) <= 3000:
            score += 10
        return score

    def add_marginalia(self, entry_id: int, png: bytes, playback_position_ms: int, theme_id: str) -> dict[str, Any]:
        if not png.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ArchiveError("invalid_marginalia_image", "Marginalia must be a PNG image.")
        self.get(entry_id)
        folder = self.root / str(int(entry_id))
        folder.mkdir(parents=True, exist_ok=True)
        next_id = int(self.connection.execute("SELECT coalesce(max(id), 0) + 1 FROM marginalia").fetchone()[0])
        filename = f"{next_id:03d}.png"
        target = folder / filename
        fd, temporary = tempfile.mkstemp(prefix=".marginalia-", dir=folder)
        try:
            with os.fdopen(fd, "wb") as handle:
                handle.write(png)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, target)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)
        relative = f"/marginalia/{entry_id}/{filename}"
        with self.connection:
            cursor = self.connection.execute(
                "INSERT INTO marginalia(archive_entry_id, image_path, created_at, playback_position_ms, theme_id) VALUES (?, ?, ?, ?, ?)",
                (int(entry_id), relative, int(time.time()), max(0, int(playback_position_ms)), str(theme_id or "shaer_dark_archive")[:80]),
            )
        return self.marginalia_page(int(cursor.lastrowid))

    def marginalia_page(self, page_id: int) -> dict[str, Any]:
        row = self.connection.execute("SELECT * FROM marginalia WHERE id = ?", (int(page_id),)).fetchone()
        if not row:
            raise ArchiveError("marginalia_not_found", "Marginalia page does not exist.", 404)
        return dict(row)

    def marginalia(self, entry_id: int) -> list[dict[str, Any]]:
        if not self.connection.execute("SELECT 1 FROM archive_entries WHERE id = ?", (int(entry_id),)).fetchone():
            raise ArchiveError("archive_entry_not_found", "Archive entry does not exist.", 404)
        return self._pages(int(entry_id))

    def _pages(self, entry_id: int) -> list[dict[str, Any]]:
        rows = self.connection.execute("SELECT * FROM marginalia WHERE archive_entry_id = ? ORDER BY created_at, id", (int(entry_id),))
        return [dict(row) for row in rows]

    def _public(self, row: sqlite3.Row) -> dict[str, Any]:
        item = dict(row)
        sources = self.connection.execute("SELECT * FROM track_sources WHERE archive_entry_id = ? ORDER BY id", (item["id"],))
        item["sources"] = [dict(source) for source in sources]
        item["marginalia_count"] = int(self.connection.execute("SELECT count(*) FROM marginalia WHERE archive_entry_id = ?", (item["id"],)).fetchone()[0])
        item["marginalia"] = self._pages(int(item["id"]))
        return item
