from __future__ import annotations

import json
import os
import stat
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from shaer_music import (
    LibraryDatabase,
    LibrespotManager,
    LoginCancelled,
    PlaybackState,
    SpotifyAuthError,
    SpotifyAuthManager,
    SpotifyApiError,
    SpotifyCache,
    SpotifyClient,
    SpotifyToken,
    TokenStore,
    TrackRepository,
    create_pkce_pair,
    spotify_playback_state,
)


class Layer12SpotifyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def auth(self, post=None):
        return SpotifyAuthManager(
            "client-id",
            "http://127.0.0.1:8775/api/spotify/callback",
            TokenStore(self.root / "token.json"),
            token_post=post or (lambda _url, _form: {}),
        )

    def test_pkce_pair_is_valid(self):
        verifier, challenge = create_pkce_pair()
        self.assertGreaterEqual(len(verifier), 43)
        self.assertLessEqual(len(verifier), 128)
        self.assertNotIn("=", verifier)
        self.assertNotIn("=", challenge)

    def test_login_persists_without_client_secret(self):
        forms = []

        def post(_url, form):
            forms.append(dict(form))
            return {
                "access_token": "access",
                "refresh_token": "refresh",
                "expires_in": 3600,
                "scope": "user-read-private",
            }

        auth = self.auth(post)
        attempt = auth.begin_login(launch_browser=False)
        token = auth.complete_login(attempt.state, "authorization-code")
        self.assertEqual(token.access_token, "access")
        self.assertNotIn("client_secret", forms[0])
        self.assertEqual(auth.token_store.load().refresh_token, "refresh")
        if os.name == "posix":
            mode = stat.S_IMODE(auth.token_store.path.stat().st_mode)
            self.assertEqual(mode, 0o600)

    def test_state_is_one_time_and_login_can_be_cancelled(self):
        auth = self.auth()
        attempt = auth.begin_login()
        self.assertTrue(auth.cancel_login(attempt.state))
        with self.assertRaises(LoginCancelled):
            auth.complete_login(attempt.state, "code")
        with self.assertRaises(SpotifyAuthError):
            auth.complete_login(attempt.state, "code")

    def test_refresh_preserves_old_refresh_token(self):
        auth = self.auth(lambda _url, _form: {"access_token": "new", "expires_in": 3600})
        auth.token_store.save(SpotifyToken("old", "refresh", int(time.time()) - 1, "scope"))
        self.assertEqual(auth.access_token(), "new")
        self.assertEqual(auth.token_store.load().refresh_token, "refresh")
        auth.logout()
        self.assertIsNone(auth.token_store.load())

    def test_client_retries_401_with_refreshed_token(self):
        class FakeAuth:
            calls = []

            def access_token(self, force_refresh=False):
                self.calls.append(force_refresh)
                return "fresh" if force_refresh else "stale"

        responses = [(401, {}, b"{}"), (200, {}, b'{"id":"user"}')]
        client = SpotifyClient(FakeAuth(), transport=lambda *_args: responses.pop(0))
        self.assertEqual(client.current_user(), {"id": "user"})
        self.assertEqual(FakeAuth.calls, [False, True])

    def test_central_client_routes_playback_commands(self):
        class FakeAuth:
            def access_token(self, force_refresh=False):
                return "token"

        requests = []

        def transport(method, url, _headers, body):
            requests.append((method, url, body))
            return 204, {}, b""

        client = SpotifyClient(FakeAuth(), transport=transport)
        client.transfer("device", play=True)
        client.seek(1200)
        client.volume(72)
        self.assertEqual([entry[0] for entry in requests], ["PUT", "PUT", "PUT"])
        self.assertIn("/me/player/seek?position_ms=1200", requests[1][1])
        self.assertIn("volume_percent=72", requests[2][1])

    def test_client_accepts_empty_and_non_json_success_responses(self):
        class FakeAuth:
            def access_token(self, force_refresh=False):
                return "token"

        responses = [(204, {}, b""), (200, {}, b'{"ok":true}'), (200, {}, b"upstream accepted")]
        client = SpotifyClient(FakeAuth(), transport=lambda *_args: responses.pop(0))
        self.assertIsNone(client.pause())
        self.assertEqual(client.current_user(), {"ok": True})
        self.assertEqual(client.current_user(), "upstream accepted")

    def test_client_loads_playlist_items_with_validated_id(self):
        class FakeAuth:
            def access_token(self, force_refresh=False):
                return "token"

        requests = []

        def transport(method, url, _headers, _body):
            requests.append((method, url))
            return 200, {}, b'{"items":[{"item":{"name":"Actual playlist track"}}]}'

        client = SpotifyClient(FakeAuth(), transport=transport)
        payload = client.playlist_items("ABC123xyz", limit=12)
        self.assertEqual(payload["items"][0]["item"]["name"], "Actual playlist track")
        self.assertIn("/playlists/ABC123xyz/items?limit=12&offset=0", requests[0][1])
        with self.assertRaisesRegex(ValueError, "valid Spotify playlist ID"):
            client.playlist_items("../not-valid")

    def test_client_preserves_spotify_error_status_message_and_reason(self):
        class FakeAuth:
            def access_token(self, force_refresh=False):
                return "token"

        for status in (401, 403):
            client = SpotifyClient(FakeAuth(), transport=lambda *_args, status=status: (status, {}, b'{"error":{"message":"Restriction violated","reason":"UNKNOWN"}}'))
            with self.assertRaises(SpotifyApiError) as raised:
                client.current_user()
            self.assertEqual(raised.exception.status, status)
            self.assertEqual(str(raised.exception), "Restriction violated")
            self.assertEqual(raised.exception.reason, "UNKNOWN")

    def test_client_preserves_transport_failures(self):
        class FakeAuth:
            def access_token(self, force_refresh=False):
                return "token"

        client = SpotifyClient(FakeAuth(), transport=lambda *_args: (_ for _ in ()).throw(SpotifyApiError(0, "Spotify is temporarily unreachable.")))
        with self.assertRaisesRegex(SpotifyApiError, "temporarily unreachable"):
            client.current_user()

    def test_playback_state_is_source_neutral(self):
        state = spotify_playback_state({
            "is_playing": True,
            "progress_ms": 12000,
            "device": {"volume_percent": 64},
            "item": {
                "name": "Song",
                "duration_ms": 192000,
                "uri": "spotify:track:1",
                "artists": [{"name": "Artist"}],
                "album": {"name": "Album", "images": [{"url": "https://image"}]},
            },
        })
        self.assertIsInstance(state, PlaybackState)
        self.assertEqual(state.artist, "Artist")
        self.assertEqual(state.source, "spotify")
        self.assertEqual(state.status, "playing")
        self.assertEqual(state.volume_percent, 64)

    def test_metadata_cache_honours_ttl(self):
        cache = SpotifyCache(self.root / "cache", metadata_ttl_s=60)
        cache.put_metadata("playlist", "one", {"name": "Mix"})
        self.assertEqual(cache.get_metadata("playlist", "one"), {"name": "Mix"})
        target = next((self.root / "cache" / "metadata").iterdir())
        wrapped = json.loads(target.read_text(encoding="utf-8"))
        wrapped["cached_at"] = 0
        target.write_text(json.dumps(wrapped), encoding="utf-8")
        self.assertIsNone(cache.get_metadata("playlist", "one"))
        self.assertEqual(cache.get_metadata("playlist", "one", allow_stale=True), {"name": "Mix"})

    def test_schema_migrates_and_merges_sources(self):
        db = LibraryDatabase(self.root / "library.db")
        db.migrate()
        columns = {row["name"] for row in db.connection.execute("PRAGMA table_info(tracks)")}
        self.assertIn("spotify_uri", columns)
        tracks = TrackRepository(db)
        track_id = tracks.upsert_spotify_only(
            "spotify-id", "spotify:track:spotify-id", "Song", "Artist", "Album", 180000
        )
        row = tracks.get(track_id)
        self.assertEqual(row["availability"], "spotify")
        self.assertEqual(row["source"], "spotify")
        db.close()

    def test_connect_status_uses_shaer_device(self):
        class FakeClient:
            def devices(self):
                return {"devices": [{"id": "device", "name": "SHAeR"}]}

        class Result:
            returncode = 0

        manager = LibrespotManager(FakeClient(), runner=lambda *_args, **_kwargs: Result())
        with patch("shaer_music.spotify_connect.shutil.which", return_value="/usr/local/bin/librespot"):
            status = manager.status()
        self.assertTrue(status.installed)
        self.assertTrue(status.discovered)
        self.assertEqual(status.device_id, "device")


if __name__ == "__main__":
    unittest.main()
