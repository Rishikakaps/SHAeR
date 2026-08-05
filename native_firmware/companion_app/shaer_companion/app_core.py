from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .artwork_cache import ArtworkCache
from .database import DatabaseManager
from .device_manager import DeviceManager
from .events import EventBus
from .firmware_manager import FirmwareManager
from .library_manager import LibraryManager
from .plugin_manager import PluginManager
from .settings_manager import SettingsManager
from .spotify_metadata import SpotifyMetadataSync
from .sync_engine import SyncEngine
from .theme_manager import ThemeManager


@dataclass
class CompanionPaths:
    root: Path

    @property
    def data(self) -> Path:
        return self.root / "data"

    @property
    def artwork(self) -> Path:
        return self.root / "artwork"

    @property
    def themes(self) -> Path:
        return self.root / "themes"


class CompanionAppCore:
    def __init__(self, root: Path) -> None:
        self.paths = CompanionPaths(root)
        self.events = EventBus()
        self.db = DatabaseManager(self.paths.data)
        self.plugins = PluginManager()
        self.library = LibraryManager(self.db, self.plugins, self.events)
        self.artwork = ArtworkCache(self.db, self.paths.artwork, self.events)
        self.device = DeviceManager(self.db, self.events)
        self.theme = ThemeManager(self.paths.themes, self.events)
        self.firmware = FirmwareManager(self.db, self.events)
        self.spotify = SpotifyMetadataSync(self.db, self.events)
        self.sync = SyncEngine(self.db, self.events)
        self.settings = SettingsManager(self.db, self.events)

    def initialize(self) -> None:
        self.paths.root.mkdir(parents=True, exist_ok=True)
        self.db.migrate()

