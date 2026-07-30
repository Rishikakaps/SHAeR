import io
import json
import base64
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


OUTPUTS = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(OUTPUTS / "shaer_pi_os"))

import server  # noqa: E402
from server import ShaerHandler  # noqa: E402


class FakeCompanion:
    def __init__(self):
        self.pairing = self
        self.track = {
            "id": 7,
            "filepath": "/music/raag.flac",
            "title": "Raag Bhairavi",
            "artist": "Archive",
            "album": "Morning",
            "duration_s": 122,
            "cover_art_path": "/music/cover.jpg",
        }

    @staticmethod
    def envelope(data):
        return {"version": 1, "ok": True, "data": data}

    def music(self, query=""):
        return self.envelope({"tracks": [self.track], "query": query})

    def playlists(self):
        return self.envelope({"playlists": [{"id": 5, "name": "Local Morning", "track_count": 1}]})

    def playlist(self, playlist_id):
        if int(playlist_id) != 5:
            raise server.ApiError("playlist_not_found", "Playlist does not exist.", 404)
        return self.envelope({"playlist": {"id": 5, "name": "Local Morning"}, "tracks": [self.track]})

    def capabilities(self, **_kwargs):
        return self.envelope({
            "capabilities": {
                "power": {"state": "permission_required"},
                "connectivity": {"state": "supported"},
            }
        })

    def authenticate(self, _token):
        return {"device_id": "test-companion"}

    def trusted_devices(self):
        return []


class FakeSpotify:
    def __init__(self):
        self.playlist_calls = []

    def playlist_items(self, playlist_id, query):
        self.playlist_calls.append((playlist_id, dict(query)))
        return {"items": [{"item": {"name": "Spotify Playlist Track"}}]}

    def status(self):
        return {"authenticated": False}

    def playback(self):
        return {}


class DeviceRouteTests(unittest.TestCase):
    def setUp(self):
        self.companion = FakeCompanion()
        self.spotify = FakeSpotify()
        with server.LOCAL_PLAYBACK_LOCK:
            server.LOCAL_PLAYBACK_STATE.clear()
            server.LOCAL_PLAYBACK_STATE.update({
                "title": "",
                "artist": "",
                "album": "",
                "duration_ms": 0,
                "progress_ms": 0,
                "cover_art": None,
                "status": "stopped",
                "queue_position": None,
                "volume_percent": None,
                "source": "local",
                "uri": None,
            })

    def handler(self, path, body=None):
        handler = object.__new__(ShaerHandler)
        handler.path = path
        handler.client_address = ("127.0.0.1", 12345)
        handler.headers = {"Content-Length": str(len(body or b""))}
        handler.rfile = io.BytesIO(body or b"")
        handler.theme = "shaer_dark_archive"
        handler.spotify = self.spotify
        handler.companion = self.companion
        handler.recording = None
        handler.allow_power = False
        handler.allow_test_input = False
        handler._suppress_log = False
        handler.response = None
        handler.error = None
        handler.send_json = lambda payload, status=200: setattr(handler, "response", (status, payload))
        handler.send_error = lambda status, *args, **kwargs: setattr(handler, "error", status)
        return handler

    def get(self, path):
        handler = self.handler(path)
        ShaerHandler.do_GET(handler)
        self.assertIsNone(handler.error)
        self.assertIsNotNone(handler.response)
        return handler.response

    def post(self, path, payload):
        body = json.dumps(payload).encode("utf-8")
        handler = self.handler(path, body)
        ShaerHandler.do_POST(handler)
        self.assertIsNone(handler.error)
        self.assertIsNotNone(handler.response)
        return handler.response

    def test_device_local_music_routes_return_real_library_payloads(self):
        status, tracks = self.get("/api/music/tracks?limit=1")
        self.assertEqual(status, 200)
        self.assertEqual(tracks["tracks"][0]["title"], "Raag Bhairavi")

        status, playlists = self.get("/api/music/playlists")
        self.assertEqual(status, 200)
        self.assertEqual(playlists["playlists"][0]["name"], "Local Morning")

        status, playlist_tracks = self.get("/api/music/playlists/5/tracks")
        self.assertEqual(status, 200)
        self.assertEqual(playlist_tracks["playlist"]["name"], "Local Morning")
        self.assertEqual(playlist_tracks["tracks"][0]["filepath"], "/music/raag.flac")

    def test_device_spotify_playlist_detail_route_reaches_spotify_runtime(self):
        status, payload = self.get("/api/spotify/playlists/ABC123xyz/tracks?limit=7")
        self.assertEqual(status, 200)
        self.assertEqual(payload["items"][0]["item"]["name"], "Spotify Playlist Track")
        self.assertEqual(self.spotify.playlist_calls[0][0], "ABC123xyz")
        self.assertEqual(self.spotify.playlist_calls[0][1]["limit"], ["7"])

    def test_device_local_playback_route_publishes_source_neutral_state(self):
        status, playback = self.post("/api/music/playback", {"action": "play-track", "id": 7})
        self.assertEqual(status, 200)
        self.assertEqual(playback["source"], "local")
        self.assertEqual(playback["status"], "playing")
        self.assertEqual(playback["title"], "Raag Bhairavi")
        self.assertEqual(playback["duration_ms"], 122000)
        self.assertEqual(playback["uri"], "local:track:7")

        status, paused = self.post("/api/music/playback", {"action": "pause"})
        self.assertEqual(status, 200)
        self.assertEqual(paused["status"], "paused")

    def test_system_capabilities_keep_legacy_flags_and_structured_states(self):
        status, payload = self.get("/api/system/capabilities")
        self.assertEqual(status, 200)
        self.assertFalse(payload["capabilities"]["power"])
        self.assertTrue(payload["capabilities"]["recording"])
        self.assertEqual(payload["capabilities"]["state"]["power"]["state"], "permission_required")

    def test_companion_wifi_status_and_scan_are_real_shaer_originated_routes(self):
        def fake_run(command, timeout_s=4.0):
            joined = " ".join(command)
            if joined == "ip -4 -o addr show wlan0":
                return subprocess.CompletedProcess(command, 0, "3: wlan0 inet 10.0.0.42/24 brd 10.0.0.255 scope global wlan0\n", "")
            if joined == "iw dev wlan0 link":
                return subprocess.CompletedProcess(command, 0, "Connected to aa:bb:cc:dd:ee:ff\n\tSSID: Studio\n\tsignal: -48 dBm\n", "")
            if joined == "nmcli -t -f NAME,TYPE connection show":
                return subprocess.CompletedProcess(command, 0, "Studio:802-11-wireless\nEthernet:802-3-ethernet\n", "")
            if joined.startswith("nmcli -t -f SSID,SIGNAL,SECURITY,IN-USE"):
                return subprocess.CompletedProcess(command, 0, "Studio:84:WPA2:*\nGuest:55:WPA2:\n", "")
            return subprocess.CompletedProcess(command, 1, "", "unexpected")

        with patch.object(server.ShaerHandler, "_command_available", staticmethod(lambda command: command in {"iw", "nmcli"})):
            with patch.object(server.ShaerHandler, "_run_probe", staticmethod(fake_run)):
                status, payload = self.get("/api/v1/network/wifi")
                self.assertEqual(status, 200)
                self.assertEqual(payload["data"]["ssid"], "Studio")
                self.assertEqual(payload["data"]["ip_address"], "10.0.0.42")
                self.assertEqual(payload["data"]["saved_networks"], ["Studio"])

                status, scan = self.get("/api/v1/network/wifi/scan")
                self.assertEqual(status, 200)
                self.assertEqual(scan["data"]["networks"][0]["ssid"], "Studio")
                self.assertTrue(scan["data"]["networks"][0]["current"])

    def test_companion_bluetooth_status_and_scan_are_real_shaer_originated_routes(self):
        def fake_run(command, timeout_s=4.0):
            joined = " ".join(command)
            if joined == "bluetoothctl show":
                return subprocess.CompletedProcess(command, 0, "Powered: yes\nDiscoverable: no\n", "")
            if joined == "bluetoothctl paired-devices":
                return subprocess.CompletedProcess(command, 0, "Device AA:BB:CC:DD:EE:FF Headphones\n", "")
            if joined == "bluetoothctl devices":
                return subprocess.CompletedProcess(command, 0, "Device AA:BB:CC:DD:EE:FF Headphones\nDevice 11:22:33:44:55:66 Keyboard\n", "")
            return subprocess.CompletedProcess(command, 1, "", "unexpected")

        with patch.object(server.ShaerHandler, "_command_available", staticmethod(lambda command: command == "bluetoothctl")):
            with patch.object(server.ShaerHandler, "_run_probe", staticmethod(fake_run)):
                status, payload = self.get("/api/v1/bluetooth")
                self.assertEqual(status, 200)
                self.assertTrue(payload["data"]["enabled"])
                self.assertEqual(payload["data"]["paired_devices"][0]["name"], "Headphones")

                status, scan = self.get("/api/v1/bluetooth/scan")
                self.assertEqual(status, 200)
                self.assertEqual(len(scan["data"]["devices"]), 2)

    def test_companion_branding_upload_validates_png_and_reads_back_icon(self):
        png = b"\x89PNG\r\n\x1a\n" + b"\0" * 32
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(server, "ROOT", Path(tmp)):
                status, payload = self.get("/api/v1/branding")
                self.assertEqual(status, 200)
                self.assertFalse(payload["data"]["custom_icon"])

                status, updated = self.post("/api/v1/branding/app-icon", {
                    "mime_type": "image/png",
                    "content_base64": base64.b64encode(png).decode("ascii"),
                })
                self.assertEqual(status, 200)
                self.assertTrue(updated["data"]["custom_icon"])
                self.assertTrue((Path(tmp) / "shaer_companion" / "icons" / "custom-app-icon.png").exists())

    def test_companion_branding_upload_rejects_non_png(self):
        with tempfile.TemporaryDirectory() as tmp:
            with patch.object(server, "ROOT", Path(tmp)):
                handler = self.handler(
                    "/api/v1/branding/app-icon",
                    json.dumps({"mime_type": "image/jpeg", "content_base64": "AAAA"}).encode("utf-8"),
                )
                ShaerHandler.do_POST(handler)
                self.assertEqual(handler.response[0], 415)
                self.assertEqual(handler.response[1]["error"]["code"], "unsupported_icon_type")

    def test_public_pairing_state_reports_unsynced_device(self):
        status, payload = self.get("/api/v1/pairing/state")
        self.assertEqual(status, 200)
        self.assertFalse(payload["data"]["paired"])
        self.assertEqual(payload["data"]["trusted_count"], 0)
        self.assertEqual(payload["data"]["download_url"], "https://github.com/Rishikakaps/SHAeR")


if __name__ == "__main__":
    unittest.main()
