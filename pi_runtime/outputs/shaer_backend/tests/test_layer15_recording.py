import json
import shutil
import tempfile
import time
import unittest
import uuid
import wave
from pathlib import Path

from shaer_recording import RecordingArchive, RecordingError, RecordingService, SyntheticCaptureBackend


class Layer15RecordingTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.archive = RecordingArchive(self.root / "recordings.db", self.root / "Recordings")
        self.service = RecordingService(
            self.archive,
            backend_factory=lambda: SyntheticCaptureBackend(seconds=0.12),
            minimum_free_bytes=1,
        )

    def tearDown(self):
        if self.service.status()["state"] != "idle":
            self.service.cancel()
        self.archive.close()
        self.temporary.cleanup()

    def record(self, theme="shaer_dark_archive"):
        self.service.start(theme)
        return self.service.stop()

    def test_record_pause_resume_and_finalize_wav_with_sidecar(self):
        started = self.service.start("shaer_japanese_punk")
        self.assertEqual(started["state"], "recording")
        self.assertEqual(self.service.pause()["state"], "paused")
        self.assertEqual(self.service.resume()["state"], "recording")
        item = self.service.stop()
        path = Path(item["file_path"])
        sidecar = Path(item["sidecar_path"])
        self.assertTrue(path.is_file())
        self.assertTrue(sidecar.is_file())
        self.assertEqual(path.parent.parent.name, time.strftime("%Y"))
        self.assertEqual(path.parent.name, time.strftime("%B"))
        with wave.open(str(path), "rb") as source:
            self.assertEqual(source.getnchannels(), 1)
            self.assertGreater(source.getnframes(), 0)
        metadata = json.loads(sidecar.read_text(encoding="utf-8"))
        self.assertEqual(metadata["device_theme"], "shaer_japanese_punk")
        self.assertEqual(metadata["status"], "complete")

    def test_cancel_removes_partial_capture(self):
        self.service.start("shaer_bombay_ticket")
        partial = self.service.partial_path
        self.assertIsNotNone(partial)
        result = self.service.cancel()
        self.assertEqual(result["state"], "idle")
        self.assertFalse(partial.exists())
        self.assertEqual(self.archive.list(), [])

    def test_start_rejects_playback_conflict_and_low_storage(self):
        with self.assertRaises(RecordingError) as conflict:
            self.service.start("shaer_dark_archive", playback_active=True)
        self.assertEqual(conflict.exception.code, "audio_mode_conflict")
        constrained = RecordingService(
            self.archive,
            backend_factory=SyntheticCaptureBackend,
            minimum_free_bytes=shutil.disk_usage(self.archive.root).total + 1,
        )
        with self.assertRaises(RecordingError) as storage:
            constrained.start("shaer_dark_archive")
        self.assertEqual(storage.exception.code, "low_storage")

    def test_archive_search_filters_and_management(self):
        item = self.record("shaer_indian_print")
        renamed = self.archive.update(item["id"], {
            "title": "Monsoon thought",
            "favorite": True,
            "notes": "Recorded beside the window",
            "playback_position_ms": 40,
        })
        self.assertTrue(renamed["favorite"])
        self.assertEqual(self.archive.list(query="window")[0]["id"], item["id"])
        self.assertEqual(self.archive.list(favorite=True)[0]["title"], "Monsoon thought")
        self.assertEqual(self.archive.list(minimum_duration_ms=100)[0]["id"], item["id"])
        self.assertEqual(self.archive.list(maximum_duration_ms=50), [])
        duplicate = self.archive.duplicate(item["id"])
        self.assertNotEqual(duplicate["recording_uuid"], item["recording_uuid"])
        moved = self.archive.move(duplicate["id"], "Field Notes")
        self.assertEqual(moved["archive_folder"], "Field_Notes")
        self.assertIn("Archive/Field_Notes", moved["file_path"])
        self.archive.delete(duplicate["id"])
        with self.assertRaises(RecordingError):
            self.archive.get(duplicate["id"])

    def test_valid_interrupted_wav_is_recovered_on_startup(self):
        folder = self.archive.root / "2026" / "July"
        folder.mkdir(parents=True)
        recording_uuid = str(uuid.uuid4())
        partial = folder / "interrupted.wav.partial"
        final = folder / "interrupted.wav"
        sidecar = folder / "interrupted.json"
        journal = folder / "interrupted.recording.json"
        with wave.open(str(partial), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(8000)
            output.writeframes(b"\x00\x00" * 800)
        journal.write_text(json.dumps({
            "phase": "recording",
            "recording_uuid": recording_uuid,
            "timestamp": int(time.time()),
            "device_theme": "shaer_ghibli_garden",
            "partial_path": str(partial),
            "final_path": str(final),
            "sidecar_path": str(sidecar),
        }), encoding="utf-8")
        RecordingService(self.archive, backend_factory=SyntheticCaptureBackend, minimum_free_bytes=1)
        recovered = self.archive.get_by_uuid(recording_uuid)
        self.assertEqual(recovered["status"], "recovered")
        self.assertTrue(final.exists())
        self.assertFalse(journal.exists())


if __name__ == "__main__":
    unittest.main()
