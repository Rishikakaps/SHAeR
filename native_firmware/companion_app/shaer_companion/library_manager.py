from __future__ import annotations

from pathlib import Path

from .database import DatabaseManager
from .events import Event, EventBus
from .models import ImportResult
from .plugin_manager import PluginManager


class LibraryManager:
    def __init__(self, db: DatabaseManager, plugins: PluginManager, events: EventBus) -> None:
        self.db = db
        self.plugins = plugins
        self.events = events

    def import_folder(self, folder: Path) -> ImportResult:
        self.events.publish(Event("library.import.started", "LibraryManager", {"folder": str(folder)}))
        result = self.plugins.import_from("local_folder", folder)
        track_count = self.db.upsert_tracks(result.tracks)
        playlist_count = self.db.upsert_playlists(result.playlists)
        self.events.publish(
            Event(
                "library.import.finished",
                "LibraryManager",
                {"tracks": track_count, "playlists": playlist_count, "errors": len(result.errors)},
            )
        )
        return result

    def tracks(self) -> list[dict]:
        return [dict(row) for row in self.db.tracks()]

    def duplicate_groups(self) -> dict[str, list[dict]]:
        groups: dict[str, list[dict]] = {}
        for track in self.tracks():
            groups.setdefault(track["content_hash"], []).append(track)
        return {key: value for key, value in groups.items() if len(value) > 1}

