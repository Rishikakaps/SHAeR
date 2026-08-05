"""SQLite index and sidecar management for SHAeR recordings."""

from __future__ import annotations

import json
import os
import shutil
import sqlite3
import tempfile
import time
import uuid
from pathlib import Path
from typing import Any, Mapping


SCHEMA_VERSION = 1
SCHEMA = """
CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS recordings (
    id INTEGER PRIMARY KEY,
    recording_uuid TEXT NOT NULL UNIQUE,
    file_path TEXT NOT NULL UNIQUE,
    sidecar_path TEXT NOT NULL UNIQUE,
    created_at INTEGER NOT NULL,
    duration_ms INTEGER NOT NULL DEFAULT 0,
    file_size INTEGER NOT NULL DEFAULT 0,
    theme TEXT NOT NULL,
    title TEXT,
    favorite INTEGER NOT NULL DEFAULT 0,
    playback_position_ms INTEGER NOT NULL DEFAULT 0,
    sync_status TEXT NOT NULL DEFAULT 'local',
    status TEXT NOT NULL DEFAULT 'complete',
    notes TEXT,
    archive_folder TEXT,
    updated_at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_recordings_created ON recordings(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_recordings_favorite ON recordings(favorite, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_recordings_title ON recordings(title);
"""


class RecordingError(RuntimeError):
    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code


class RecordingArchive:
    def __init__(self, database_path: str | Path, recordings_root: str | Path):
        self.database_path = Path(database_path).expanduser()
        self.root = Path(recordings_root).expanduser()
        self.database_path.parent.mkdir(parents=True, exist_ok=True)
        self.root.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(self.database_path, check_same_thread=False)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys = ON")
        self.connection.execute("PRAGMA journal_mode = WAL")
        self.connection.execute("PRAGMA synchronous = FULL")
        self.connection.execute("PRAGMA busy_timeout = 5000")
        self.connection.execute("PRAGMA wal_autocheckpoint = 1000")
        self.migrate()

    def migrate(self) -> None:
        with self.connection:
            self.connection.executescript(SCHEMA)
            self.connection.execute(
                "INSERT OR IGNORE INTO schema_migrations(version, applied_at) VALUES (?, ?)",
                (SCHEMA_VERSION, int(time.time())),
            )

    def close(self) -> None:
        self.connection.close()

    def add(self, metadata: Mapping[str, Any]) -> dict[str, Any]:
        now = int(time.time())
        payload = {
            "recording_uuid": str(metadata["recording_uuid"]),
            "file_path": str(metadata["file_path"]),
            "sidecar_path": str(metadata["sidecar_path"]),
            "created_at": int(metadata["timestamp"]),
            "duration_ms": int(metadata.get("duration_ms") or 0),
            "file_size": int(metadata.get("file_size") or 0),
            "theme": str(metadata.get("device_theme") or "shaer_dark_archive"),
            "title": metadata.get("title"),
            "favorite": 1 if metadata.get("favorite") else 0,
            "playback_position_ms": int(metadata.get("playback_position_ms") or 0),
            "sync_status": str(metadata.get("sync_status") or "local"),
            "status": str(metadata.get("status") or "complete"),
            "notes": metadata.get("notes"),
            "archive_folder": metadata.get("archive_folder"),
            "updated_at": now,
        }
        with self.connection:
            self.connection.execute(
                """
                INSERT INTO recordings(
                    recording_uuid, file_path, sidecar_path, created_at, duration_ms,
                    file_size, theme, title, favorite, playback_position_ms,
                    sync_status, status, notes, archive_folder, updated_at
                ) VALUES (
                    :recording_uuid, :file_path, :sidecar_path, :created_at, :duration_ms,
                    :file_size, :theme, :title, :favorite, :playback_position_ms,
                    :sync_status, :status, :notes, :archive_folder, :updated_at
                )
                ON CONFLICT(recording_uuid) DO UPDATE SET
                    file_path=excluded.file_path, sidecar_path=excluded.sidecar_path,
                    duration_ms=excluded.duration_ms, file_size=excluded.file_size,
                    theme=excluded.theme, title=excluded.title, favorite=excluded.favorite,
                    playback_position_ms=excluded.playback_position_ms,
                    sync_status=excluded.sync_status, status=excluded.status,
                    notes=excluded.notes, archive_folder=excluded.archive_folder,
                    updated_at=excluded.updated_at
                """,
                payload,
            )
        return self.get_by_uuid(payload["recording_uuid"])

    def get(self, recording_id: int) -> dict[str, Any]:
        row = self.connection.execute("SELECT * FROM recordings WHERE id = ?", (recording_id,)).fetchone()
        if not row:
            raise RecordingError("recording_not_found", "Recording does not exist.")
        return self._public(row)

    def get_by_uuid(self, recording_uuid: str) -> dict[str, Any]:
        row = self.connection.execute("SELECT * FROM recordings WHERE recording_uuid = ?", (recording_uuid,)).fetchone()
        if not row:
            raise RecordingError("recording_not_found", "Recording does not exist.")
        return self._public(row)

    def list(
        self,
        query: str = "",
        favorite: bool | None = None,
        year: int | None = None,
        month: int | None = None,
        minimum_duration_ms: int | None = None,
        maximum_duration_ms: int | None = None,
        limit: int = 200,
    ) -> list[dict[str, Any]]:
        where: list[str] = []
        values: list[Any] = []
        if query.strip():
            where.append("(lower(coalesce(title, '')) LIKE ? OR lower(coalesce(notes, '')) LIKE ? OR date(created_at, 'unixepoch') LIKE ?)")
            pattern = f"%{query.strip().lower()}%"
            values.extend((pattern, pattern, pattern))
        if favorite is not None:
            where.append("favorite = ?")
            values.append(1 if favorite else 0)
        if year is not None:
            where.append("strftime('%Y', created_at, 'unixepoch') = ?")
            values.append(f"{year:04d}")
        if month is not None:
            where.append("strftime('%m', created_at, 'unixepoch') = ?")
            values.append(f"{month:02d}")
        if minimum_duration_ms is not None:
            where.append("duration_ms >= ?")
            values.append(max(0, int(minimum_duration_ms)))
        if maximum_duration_ms is not None:
            where.append("duration_ms <= ?")
            values.append(max(0, int(maximum_duration_ms)))
        clause = " WHERE " + " AND ".join(where) if where else ""
        values.append(min(1000, max(1, limit)))
        rows = self.connection.execute(
            "SELECT * FROM recordings" + clause + " ORDER BY created_at DESC LIMIT ?",
            values,
        )
        return [self._public(row) for row in rows]

    def update(self, recording_id: int, patch: Mapping[str, Any]) -> dict[str, Any]:
        allowed = {"title", "favorite", "playback_position_ms", "sync_status", "notes"}
        if not patch or any(key not in allowed for key in patch):
            raise RecordingError("invalid_recording_update", "Recording update contains unsupported fields.")
        current = self.get(recording_id)
        sanitized: dict[str, Any] = {}
        for key, value in patch.items():
            if key == "title":
                sanitized[key] = str(value).strip()[:120] or None
            elif key == "favorite":
                sanitized[key] = 1 if bool(value) else 0
            elif key == "playback_position_ms":
                sanitized[key] = max(0, int(value))
            elif key == "sync_status":
                if str(value) not in {"local", "pending", "synced", "conflict"}:
                    raise RecordingError("invalid_sync_status", "Invalid recording sync state.")
                sanitized[key] = str(value)
            elif key == "notes":
                sanitized[key] = str(value)[:2000] if value is not None else None
        assignments = ", ".join(f"{key} = ?" for key in sanitized)
        with self.connection:
            self.connection.execute(
                f"UPDATE recordings SET {assignments}, updated_at = ? WHERE id = ?",
                (*sanitized.values(), int(time.time()), recording_id),
            )
        updated = self.get(recording_id)
        self.write_sidecar(updated)
        return updated

    def delete(self, recording_id: int) -> None:
        item = self.get(recording_id)
        for key in ("file_path", "sidecar_path"):
            path = Path(str(item[key]))
            if self._within(path, self.root):
                path.unlink(missing_ok=True)
        with self.connection:
            self.connection.execute("DELETE FROM recordings WHERE id = ?", (recording_id,))

    def duplicate(self, recording_id: int) -> dict[str, Any]:
        source = self.get(recording_id)
        source_file = Path(str(source["file_path"]))
        if not source_file.exists():
            raise RecordingError("recording_file_missing", "Recording audio file is missing.")
        new_uuid = str(uuid.uuid4())
        target = source_file.with_name(f"{source_file.stem}_copy_{new_uuid[:8]}{source_file.suffix}")
        shutil.copy2(source_file, target)
        sidecar = target.with_suffix(".json")
        metadata = self._metadata_from_public(source)
        metadata.update({
            "recording_uuid": new_uuid,
            "file_path": str(target),
            "sidecar_path": str(sidecar),
            "timestamp": int(time.time()),
            "title": ((source.get("title") or "Recording") + " copy")[:120],
            "file_size": target.stat().st_size,
            "sync_status": "local",
        })
        self._atomic_json(sidecar, metadata)
        return self.add(metadata)

    def move(self, recording_id: int, folder: str) -> dict[str, Any]:
        clean = "_".join(part for part in folder.strip().replace("/", " ").split() if part)[:48]
        if not clean:
            raise RecordingError("invalid_archive_folder", "Archive folder name is required.")
        item = self.get(recording_id)
        source = Path(str(item["file_path"]))
        sidecar = Path(str(item["sidecar_path"]))
        target_dir = self.root / "Archive" / clean
        target_dir.mkdir(parents=True, exist_ok=True)
        target = target_dir / source.name
        target_sidecar = target_dir / sidecar.name
        if target.exists() or target_sidecar.exists():
            raise RecordingError("archive_conflict", "A recording with this name already exists in that folder.")
        os.replace(source, target)
        if sidecar.exists():
            os.replace(sidecar, target_sidecar)
        with self.connection:
            self.connection.execute(
                "UPDATE recordings SET file_path = ?, sidecar_path = ?, archive_folder = ?, updated_at = ? WHERE id = ?",
                (str(target), str(target_sidecar), clean, int(time.time()), recording_id),
            )
        updated = self.get(recording_id)
        self.write_sidecar(updated)
        return updated

    def storage(self) -> dict[str, int]:
        usage = shutil.disk_usage(self.root)
        total_recording_bytes = self.connection.execute("SELECT coalesce(sum(file_size), 0) FROM recordings").fetchone()[0]
        return {"total": usage.total, "used": usage.used, "free": usage.free, "recordings": int(total_recording_bytes)}

    def write_sidecar(self, item: Mapping[str, Any]) -> None:
        self._atomic_json(Path(str(item["sidecar_path"])), self._metadata_from_public(item))

    @staticmethod
    def _public(row: sqlite3.Row) -> dict[str, Any]:
        item = dict(row)
        item["favorite"] = bool(item["favorite"])
        item["display_title"] = item["title"] or time.strftime("Recording %d %b %Y, %H:%M", time.localtime(item["created_at"]))
        item["date"] = time.strftime("%Y-%m-%d", time.localtime(item["created_at"]))
        return item

    @staticmethod
    def _metadata_from_public(item: Mapping[str, Any]) -> dict[str, Any]:
        return {
            "schema_version": 1,
            "recording_uuid": item["recording_uuid"],
            "timestamp": int(item.get("created_at") or item.get("timestamp") or time.time()),
            "duration_ms": int(item.get("duration_ms") or 0),
            "file_size": int(item.get("file_size") or 0),
            "device_theme": item.get("theme") or item.get("device_theme") or "shaer_dark_archive",
            "title": item.get("title"),
            "favorite": bool(item.get("favorite")),
            "playback_position_ms": int(item.get("playback_position_ms") or 0),
            "sync_status": item.get("sync_status") or "local",
            "status": item.get("status") or "complete",
            "notes": item.get("notes"),
            "archive_folder": item.get("archive_folder"),
            "file_path": str(item["file_path"]),
            "sidecar_path": str(item["sidecar_path"]),
            "transcript": None,
            "transcript_source": None,
        }

    @staticmethod
    def _atomic_json(path: Path, payload: Mapping[str, Any]) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix=".shaer-recording-", dir=path.parent)
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as handle:
                json.dump(payload, handle, indent=2, sort_keys=True)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, path)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

    @staticmethod
    def _within(path: Path, root: Path) -> bool:
        try:
            path.resolve().relative_to(root.resolve())
            return True
        except ValueError:
            return False
