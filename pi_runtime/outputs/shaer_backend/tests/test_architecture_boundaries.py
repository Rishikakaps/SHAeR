import tempfile
import time
import unittest
from pathlib import Path

from shaer_hal import GpioInputController, InputAction, SimulatedInputController
from shaer_music import (
    LibraryDatabase,
    LocalLibraryProvider,
    ProviderRegistry,
    TrackRecord,
    TrackRepository,
)


class ArchitectureBoundaryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.database = LibraryDatabase(self.root / "library.db")
        self.database.migrate()

    def tearDown(self):
        self.database.close()
        self.temporary.cleanup()

    def test_local_library_uses_source_neutral_provider_contract(self):
        tracks = TrackRepository(self.database)
        tracks.upsert(TrackRecord(
            filepath=str(self.root / "alaap.flac"),
            title="Alaap",
            artist="Yaman",
            album="Night",
            duration_s=180,
        ))
        registry = ProviderRegistry([LocalLibraryProvider(tracks)])
        item = registry.browse("local")[0]
        self.assertEqual((item.provider, item.title, item.duration_ms), ("local", "Alaap", 180000))
        self.assertEqual(registry.search("yaman")[0].media_id, item.media_id)
        self.assertTrue(registry.capabilities()["local"].playback)

    def test_provider_ids_are_unique_and_unknown_sources_fail_closed(self):
        provider = LocalLibraryProvider(TrackRepository(self.database))
        registry = ProviderRegistry([provider])
        with self.assertRaises(ValueError):
            registry.register(provider)
        with self.assertRaises(LookupError):
            registry.browse("missing")

    def test_simulated_encoder_uses_same_semantic_events_as_gpio(self):
        received = []
        controller = SimulatedInputController(received.append)
        for action in (InputAction.RIGHT, InputAction.SELECT, InputAction.BACK):
            controller.emit(action)
        self.assertEqual([event.action for event in received], [
            InputAction.RIGHT,
            InputAction.SELECT,
            InputAction.BACK,
        ])
        self.assertTrue(all(event.source == "simulated" for event in received))

    def test_gpio_ok_double_click_switches_encoder_mode(self):
        received = []
        controller = GpioInputController(received.append, pin_a=17, pin_b=27, pin_select=22, pin_back=23)
        controller._pressed("select")
        controller._released("select", InputAction.SELECT, InputAction.LONG_SELECT)
        controller._pressed("select")
        controller._released("select", InputAction.SELECT, InputAction.LONG_SELECT)
        self.assertEqual([event.action for event in received], [InputAction.TOGGLE_INPUT_MODE])
        controller._pressed("select")
        controller._released("select", InputAction.SELECT, InputAction.LONG_SELECT)
        time.sleep(0.36)
        self.assertEqual([event.action for event in received], [InputAction.TOGGLE_INPUT_MODE, InputAction.SELECT])
        controller.close()

    def test_sqlite_uses_power_loss_resistant_pragmas(self):
        synchronous = int(self.database.connection.execute("PRAGMA synchronous").fetchone()[0])
        timeout = int(self.database.connection.execute("PRAGMA busy_timeout").fetchone()[0])
        journal = str(self.database.connection.execute("PRAGMA journal_mode").fetchone()[0]).lower()
        self.assertEqual(synchronous, 2)
        self.assertGreaterEqual(timeout, 5000)
        self.assertEqual(journal, "wal")


if __name__ == "__main__":
    unittest.main()
