import tempfile
import unittest
from pathlib import Path

from shaer_archive import ArchiveError, MusicalArchive


class MarginaliaArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = Path(self.temp.name)
        self.archive = MusicalArchive(root / "archive.db", root / "marginalia")

    def tearDown(self):
        self.archive.close()
        self.temp.cleanup()

    def test_entry_survives_source_and_stores_multiple_pages(self):
        entry = self.archive.create_or_get({
            "title": "505", "artist": "Arctic Monkeys", "album": "Favourite Worst Nightmare",
            "duration_ms": 253000, "uri": "spotify:track:505",
        })
        self.archive.add_marginalia(entry["id"], b"\x89PNG\r\n\x1a\npage-one", 188400, "shaer_dark_archive")
        self.archive.add_marginalia(entry["id"], b"\x89PNG\r\n\x1a\npage-two", 200000, "shaer_japanese_punk")
        loaded = self.archive.get(entry["id"])
        self.assertEqual(loaded["marginalia_count"], 2)
        self.assertEqual(loaded["sources"][0]["source_type"], "spotify")
        self.assertTrue(loaded["marginalia"][0]["image_path"].endswith("/001.png"))

    def test_local_source_matching_is_conservative(self):
        entry = self.archive.create_or_get({
            "title": "505", "artist": "Arctic Monkeys", "album": "Favourite Worst Nightmare",
            "duration_ms": 253000, "uri": "spotify:track:505",
        })
        self.assertEqual(self.archive.match_score(entry, {
            "title": "505", "artist": "Arctic Monkeys", "album": "Favourite Worst Nightmare",
            "duration_ms": 253100, "filepath": "/Music/505.flac",
        }), 100)
        self.assertEqual(self.archive.match_score(entry, {
            "title": "505 Live", "artist": "Arctic Monkeys", "album": "Favourite Worst Nightmare",
            "duration_ms": 253100, "filepath": "/Music/505-live.flac",
        }), 60)
        with self.assertRaises(ArchiveError):
            self.archive.add_source(entry["id"], {"title": "505 Live", "artist": "Arctic Monkeys", "album": "Favourite Worst Nightmare", "duration_ms": 253100, "filepath": "/Music/505-live.flac"})


if __name__ == "__main__":
    unittest.main()
