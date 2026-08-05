import tempfile
import unittest
from pathlib import Path

from shaer_music import (
    LibraryDatabase,
    LibraryIndexer,
    LocalLibraryMatcher,
    PlaybackQueue,
    PlaylistRepository,
    QueueRepository,
    RepeatMode,
    SessionRepository,
    SpotifyTrack,
    TrackRecord,
    TrackRepository,
)


class Layer11MusicTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.db = LibraryDatabase(self.root / "library.db")
        self.db.migrate()

    def tearDown(self):
        self.db.close()
        self.tmp.cleanup()

    def test_schema_and_track_repository(self):
        tracks = TrackRepository(self.db)
        track_id = tracks.upsert(
            TrackRecord(
                filepath=str(self.root / "song.mp3"),
                title="Alaap",
                artist="Yaman",
                album="Night",
                duration_s=180,
                isrc="ISRC1",
            )
        )
        self.assertEqual(tracks.get(track_id)["title"], "Alaap")
        self.assertEqual(tracks.artists(), ["Yaman"])
        self.assertEqual(tracks.albums(), ["Night"])

    def test_playlist_repository(self):
        tracks = TrackRepository(self.db)
        track_id = tracks.upsert(TrackRecord(filepath=str(self.root / "song.flac"), title="Track"))
        playlists = PlaylistRepository(self.db)
        playlist_id = playlists.create("Liked Songs")
        playlists.add_track(playlist_id, track_id, 1)
        self.assertEqual(playlists.tracks(playlist_id)[0]["id"], track_id)

    def test_queue_snapshot_and_repeat(self):
        queue = PlaybackQueue([1, 2], repeat_mode=RepeatMode.ALL)
        self.assertEqual(queue.next_track_id(), 2)
        self.assertEqual(queue.next_track_id(), 1)
        store = QueueRepository(self.root / "last_queue_state.json")
        store.save(queue)
        restored = store.load()
        self.assertEqual(restored.track_ids, [1, 2])
        self.assertEqual(restored.repeat_mode, RepeatMode.ALL)

    def test_statistics_session_updates_track(self):
        tracks = TrackRepository(self.db)
        track_id = tracks.upsert(TrackRecord(filepath=str(self.root / "song.ogg"), title="Track"))
        sessions = SessionRepository(self.db)
        sessions.record_listen(track_id, 45, "local")
        row = tracks.get(track_id)
        self.assertEqual(row["play_count"], 1)
        self.assertEqual(row["total_listened_s"], 45)
        self.assertEqual(len(sessions.history()), 1)

    def test_indexer_scans_supported_files(self):
        music = self.root / "music"
        music.mkdir()
        song = music / "demo.mp3"
        song.write_bytes(b"not real audio but enough for fallback metadata")
        report = LibraryIndexer(self.db, music).index()
        self.assertEqual(report["scanned"], 1)
        self.assertEqual(report["indexed"], 1)
        self.assertEqual(TrackRepository(self.db).all()[0]["title"], "demo")

    def test_spotify_matcher(self):
        matcher = LocalLibraryMatcher()
        spotify = SpotifyTrack(
            spotify_track_id="spotify:track:1",
            title="Alaap",
            artist="Yaman",
            album="Night",
            duration_ms=180000,
            isrc="ISRC1",
        )
        local = {"title": "Alaap", "artist": "Yaman", "album": "Night", "duration_s": 180, "isrc": "ISRC1"}
        self.assertEqual(matcher.confidence(spotify, local), 100)
        self.assertTrue(matcher.is_auto_match(spotify, local))


if __name__ == "__main__":
    unittest.main()
