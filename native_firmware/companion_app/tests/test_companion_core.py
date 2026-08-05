from __future__ import annotations

import shutil
import sqlite3
import sys
import tempfile
import unittest
import wave
from contextlib import closing
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from shaer_companion.app_core import CompanionAppCore
from shaer_companion.events import Event, EventBus
from shaer_companion.providers.local_folder import LocalFolderProvider
from shaer_companion.web_ui import CompanionWebServer, HTML


def write_wav(path: Path) -> None:
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(44100)
        wav.writeframes(b"\x00\x00" * 4410)


class CompanionCoreTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp(prefix="shaer_companion_test_"))
        self.core = CompanionAppCore(self.tmp / "app")
        self.core.initialize()

    def tearDown(self) -> None:
        shutil.rmtree(self.tmp)

    def test_event_bus_drains_to_subscriber(self) -> None:
        bus = EventBus()
        seen: list[str] = []
        bus.subscribe("x", lambda event: seen.append(event.source))
        bus.publish(Event("x", "unit"))
        self.assertEqual(bus.pending_count(), 1)
        bus.drain()
        self.assertEqual(seen, ["unit"])

    def test_local_folder_provider_imports_wav(self) -> None:
        music = self.tmp / "music"
        music.mkdir()
        write_wav(music / "Demo Song.wav")
        result = LocalFolderProvider().scan(music)
        self.assertEqual(len(result.tracks), 1)
        self.assertEqual(result.tracks[0].title, "Demo Song")
        self.assertEqual(result.tracks[0].file_format, "wav")
        self.assertGreater(result.tracks[0].duration_seconds, 0)

    def test_library_manager_persists_import(self) -> None:
        music = self.tmp / "music"
        music.mkdir()
        write_wav(music / "Demo Song.wav")
        result = self.core.library.import_folder(music)
        self.assertEqual(len(result.tracks), 1)
        tracks = self.core.library.tracks()
        self.assertEqual(len(tracks), 1)
        self.assertEqual(tracks[0]["title"], "Demo Song")

    def test_sync_engine_copies_incrementally_and_generates_device_db(self) -> None:
        music = self.tmp / "music"
        device = self.tmp / "device"
        music.mkdir()
        device.mkdir()
        write_wav(music / "Demo Song.wav")
        self.core.library.import_folder(music)
        first = self.core.sync.execute(device)
        second = self.core.sync.plan(device)
        self.assertEqual(len(first.items), 1)
        self.assertEqual(len(second.items), 0)
        self.assertEqual(second.skipped_duplicates, 1)
        library_db = device / "shaer" / "data" / "library.db"
        self.assertTrue(library_db.exists())
        with closing(sqlite3.connect(library_db)) as conn:
            count = conn.execute("SELECT COUNT(*) FROM tracks").fetchone()[0]
        self.assertEqual(count, 1)

    def test_settings_backup_restore(self) -> None:
        backup = self.tmp / "settings.json"
        self.core.settings.set("wifi.ssid", "Studio")
        self.core.settings.backup(backup)
        self.assertTrue(backup.exists())
        restored = self.core.settings.restore(backup)
        self.assertEqual(restored, 1)

    def test_theme_install_and_customize(self) -> None:
        theme = self.tmp / "theme"
        theme.mkdir()
        (theme / "theme.json").write_text('{"id":"test_theme","display_name":"Test Theme","colors":{}}\n')
        installed = self.core.theme.install_theme(theme)
        self.assertTrue(installed.exists())
        self.core.theme.customize_colors("test_theme", {"accent": "#123456"})
        themes = self.core.theme.themes()
        self.assertEqual(themes[0]["colors"]["accent"], "#123456")

    def test_firmware_integrity_and_staging(self) -> None:
        package = self.tmp / "firmware.bin"
        device = self.tmp / "device"
        device.mkdir()
        package.write_bytes(b"firmware")
        digest = self.core.firmware.register_firmware("1.0.0", package)
        staged = self.core.firmware.stage_update("1.0.0", device)
        self.assertTrue(staged.exists())
        self.assertEqual(len(digest), 64)

    def test_web_ui_fallback_is_available_without_tkinter(self) -> None:
        server = CompanionWebServer(self.core, port=0)
        self.assertEqual(server.host, "127.0.0.1")
        self.assertEqual(server.port, 0)
        self.assertIn("SHAeR Companion", HTML)
        self.assertIn("/api/tracks", HTML)


if __name__ == "__main__":
    unittest.main()
