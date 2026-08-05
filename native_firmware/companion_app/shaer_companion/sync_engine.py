from __future__ import annotations

import hashlib
import shutil
import sqlite3
from contextlib import closing
from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus
from .models import SyncPlan, SyncPlanItem


def safe_name(value: str) -> str:
    cleaned = "".join(c if c.isalnum() or c in " ._-" else "_" for c in value).strip()
    return cleaned or "Unknown"


def existing_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


class SyncEngine:
    def __init__(self, db: DatabaseManager, events: EventBus) -> None:
        self.db = db
        self.events = events

    def plan(self, device_root: Path) -> SyncPlan:
        music_root = device_root / "shaer" / "music"
        items: list[SyncPlanItem] = []
        skipped = 0
        for track in self.db.tracks():
            source = Path(track["source_path"])
            destination = (
                music_root
                / safe_name(track["artist"])
                / safe_name(track["album"])
                / f"{track['content_hash'][:12]}_{safe_name(track['title'])}.{track['file_format']}"
            )
            if destination.exists() and existing_hash(destination) == track["content_hash"]:
                skipped += 1
                continue
            items.append(
                SyncPlanItem(
                    track_hash=track["content_hash"],
                    source_path=source,
                    destination_path=destination,
                    action="copy",
                    bytes_to_copy=int(track["file_size"]),
                )
            )
        return SyncPlan(items=items, skipped_duplicates=skipped)

    def execute(self, device_root: Path) -> SyncPlan:
        plan = self.plan(device_root)
        self.events.publish(Event("sync.started", "SyncEngine", {"items": len(plan.items), "bytes": plan.total_bytes}))
        for item in plan.items:
            item.destination_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item.source_path, item.destination_path)
            self.events.publish(Event("sync.file.copied", "SyncEngine", {"path": str(item.destination_path)}))
        self._generate_device_database(device_root)
        with self.db.connect(self.db.sync_db) as conn:
            conn.execute(
                "INSERT INTO sync_runs(device_path, copied_files, copied_bytes, status) VALUES (?, ?, ?, ?)",
                (str(device_root), len(plan.items), plan.total_bytes, "ok"),
            )
        self.events.publish(Event("sync.finished", "SyncEngine", {"items": len(plan.items), "bytes": plan.total_bytes}))
        return plan

    def _generate_device_database(self, device_root: Path) -> None:
        db_path = device_root / "shaer" / "data" / "library.db"
        db_path.parent.mkdir(parents=True, exist_ok=True)
        with closing(sqlite3.connect(db_path)) as conn:
            with conn:
                conn.execute("DROP TABLE IF EXISTS tracks")
                conn.execute(
                    """
                    CREATE TABLE tracks (
                        content_hash TEXT PRIMARY KEY,
                        title TEXT NOT NULL,
                        artist TEXT NOT NULL,
                        album TEXT NOT NULL,
                        genre TEXT,
                        year TEXT,
                        file_path TEXT NOT NULL,
                        spotify_uri TEXT
                    )
                    """
                )
                for track in self.db.tracks():
                    file_path = (
                        Path("music")
                        / safe_name(track["artist"])
                        / safe_name(track["album"])
                        / f"{track['content_hash'][:12]}_{safe_name(track['title'])}.{track['file_format']}"
                    )
                    conn.execute(
                        "INSERT INTO tracks(content_hash, title, artist, album, genre, year, file_path, spotify_uri) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                        (
                            track["content_hash"],
                            track["title"],
                            track["artist"],
                            track["album"],
                            track["genre"],
                            track["year"],
                            str(file_path),
                            track["spotify_uri"],
                        ),
                    )
