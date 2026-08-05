from __future__ import annotations

import shutil
from dataclasses import dataclass
from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus


@dataclass(frozen=True)
class DeviceInfo:
    path: Path
    total_bytes: int
    free_bytes: int
    firmware_version: str


class DeviceManager:
    def __init__(self, db: DatabaseManager, events: EventBus) -> None:
        self.db = db
        self.events = events

    def inspect(self, path: Path) -> DeviceInfo:
        usage = shutil.disk_usage(path)
        version_file = path / "shaer" / "firmware_version.txt"
        version = version_file.read_text(encoding="utf-8").strip() if version_file.exists() else "unknown"
        info = DeviceInfo(path=path, total_bytes=usage.total, free_bytes=usage.free, firmware_version=version)
        with self.db.connect(self.db.settings_db) as conn:
            conn.execute(
                "INSERT INTO devices(id, name, mount_path, firmware_version, last_seen) VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP) "
                "ON CONFLICT(id) DO UPDATE SET mount_path=excluded.mount_path, firmware_version=excluded.firmware_version, last_seen=CURRENT_TIMESTAMP",
                (str(path.resolve()), "SHAeR SD Card", str(path), version),
            )
        self.events.publish(Event("device.inspected", "DeviceManager", {"path": str(path), "free_bytes": info.free_bytes}))
        return info

