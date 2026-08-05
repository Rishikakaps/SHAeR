from __future__ import annotations

import json
from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus


class SettingsManager:
    def __init__(self, db: DatabaseManager, events: EventBus) -> None:
        self.db = db
        self.events = events

    def set(self, key: str, value: str) -> None:
        self.db.set_setting(key, value)
        self.events.publish(Event("settings.changed", "SettingsManager", {"key": key}))

    def backup(self, destination: Path) -> Path:
        with self.db.connect(self.db.settings_db) as conn:
            rows = {row["key"]: row["value"] for row in conn.execute("SELECT key, value FROM settings")}
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
        self.events.publish(Event("settings.backup.created", "SettingsManager", {"path": str(destination)}))
        return destination

    def restore(self, source: Path) -> int:
        values = json.loads(source.read_text(encoding="utf-8"))
        for key, value in values.items():
            self.db.set_setting(str(key), str(value))
        self.events.publish(Event("settings.restored", "SettingsManager", {"count": len(values)}))
        return len(values)

