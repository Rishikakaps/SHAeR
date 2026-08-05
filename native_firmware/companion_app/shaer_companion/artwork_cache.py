from __future__ import annotations

import hashlib
import shutil
from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus


class ArtworkCache:
    def __init__(self, db: DatabaseManager, cache_dir: Path, events: EventBus) -> None:
        self.db = db
        self.cache_dir = cache_dir
        self.events = events
        self.cache_dir.mkdir(parents=True, exist_ok=True)

    def import_artwork(self, path: Path) -> Path:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        destination = self.cache_dir / f"{digest}{path.suffix.lower()}"
        if not destination.exists():
            shutil.copy2(path, destination)
        with self.db.connect(self.db.artwork_db) as conn:
            conn.execute(
                "INSERT INTO artwork(cache_key, source, local_path) VALUES (?, ?, ?) "
                "ON CONFLICT(cache_key) DO UPDATE SET local_path=excluded.local_path, updated_at=CURRENT_TIMESTAMP",
                (digest, str(path), str(destination)),
            )
        self.events.publish(Event("artwork.cached", "ArtworkCache", {"path": str(destination)}))
        return destination

