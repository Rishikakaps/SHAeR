from __future__ import annotations

import hashlib
import shutil
from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus


class FirmwareManager:
    def __init__(self, db: DatabaseManager, events: EventBus) -> None:
        self.db = db
        self.events = events

    def register_firmware(self, version: str, package: Path, compatible_from: str = "") -> str:
        digest = hashlib.sha256(package.read_bytes()).hexdigest()
        with self.db.connect(self.db.firmware_db) as conn:
            conn.execute(
                "INSERT INTO firmware_versions(version, path, sha256, compatible_from) VALUES (?, ?, ?, ?) "
                "ON CONFLICT(version) DO UPDATE SET path=excluded.path, sha256=excluded.sha256, compatible_from=excluded.compatible_from",
                (version, str(package), digest, compatible_from),
            )
        self.events.publish(Event("firmware.registered", "FirmwareManager", {"version": version}))
        return digest

    def stage_update(self, version: str, device_path: Path) -> Path:
        with self.db.connect(self.db.firmware_db) as conn:
            row = conn.execute("SELECT path, sha256 FROM firmware_versions WHERE version = ?", (version,)).fetchone()
        if row is None:
            raise ValueError(f"firmware version not registered: {version}")
        source = Path(row["path"])
        if hashlib.sha256(source.read_bytes()).hexdigest() != row["sha256"]:
            raise ValueError("firmware package integrity check failed")
        destination = device_path / "shaer" / "updates" / source.name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        self.events.publish(Event("firmware.update.staged", "FirmwareManager", {"version": version, "path": str(destination)}))
        return destination

