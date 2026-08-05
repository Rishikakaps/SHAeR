import base64
import hashlib
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path


OUTPUTS = Path(__file__).resolve().parents[2]
BACKEND = OUTPUTS / "shaer_backend"
for path in (OUTPUTS, BACKEND):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from shaer_companion import ApiError, CompanionService  # noqa: E402
from shaer_recording import RecordingArchive, RecordingService, SyntheticCaptureBackend  # noqa: E402


class CompanionProtocolTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name) / "outputs"
        self.root.mkdir()
        self.config = Path(self.temporary.name) / "config"
        self.service = CompanionService(self.root, self.config)

    def tearDown(self):
        self.temporary.cleanup()

    def test_pairing_requires_device_approval_and_issues_trusted_token(self):
        started = self.service.pairing.start("Rishika's phone")
        self.assertRegex(started["code"], r"^\d{6}$")
        self.assertEqual(self.service.pairing.status(started["pairing_id"])["state"], "pending")
        self.service.pairing.approve(started["pairing_id"], True)
        paired = self.service.pairing.status(started["pairing_id"])
        self.assertEqual(paired["state"], "paired")
        trusted = self.service.pairing.authenticate(paired["token"])
        self.assertEqual(trusted["name"], "Rishika's phone")
        self.assertEqual(trusted["role"], "owner")
        self.assertEqual(paired["device"]["id"], trusted["id"])
        self.assertTrue(self.service.pairing.forget(trusted["id"]))
        with self.assertRaises(ApiError):
            self.service.pairing.authenticate(paired["token"])
        with self.assertRaises(ApiError):
            self.service.pairing.authenticate("not-a-token")

    def test_settings_are_versioned_and_unknown_keys_are_rejected(self):
        result = self.service.update_settings({"display": {"brightness": 42}})
        self.assertEqual(result["version"], 1)
        self.assertEqual(result["data"]["display"]["brightness"], 42)
        with self.assertRaises(ApiError):
            self.service.update_settings({"display": {"mystery": True}})

    def test_music_upload_indexes_track_and_playlist_changes_without_reboot(self):
        uploaded = self.service.upload_track("A Song.mp3", base64.b64encode(b"ID3\x04\x00\x00\x00\x00\x00\x00").decode("ascii"))
        self.assertEqual(uploaded["data"]["index"]["indexed"], 1)
        track = self.service.music()["data"]["tracks"][0]
        playlist = self.service.create_playlist("Daily", [track["id"]])["data"]
        self.assertEqual(playlist["playlist"]["name"], "Daily")
        self.assertEqual(len(playlist["tracks"]), 1)
        self.service.update_track(track["id"], {"artist": "SHAeR"})
        self.assertEqual(self.service.music()["data"]["tracks"][0]["artist"], "SHAeR")

    def test_encrypted_backup_round_trip_and_wrong_passphrase_rejected(self):
        self.service.update_settings({"display": {"brightness": 33}})
        backup = self.service.create_backup("correct horse", ["settings"])["data"]
        self.service.update_settings({"display": {"brightness": 88}})
        self.service.restore_backup(backup["content_base64"], "correct horse", ["settings"])
        self.assertEqual(self.service.settings()["data"]["display"]["brightness"], 33)
        with self.assertRaises(ApiError):
            self.service.restore_backup(backup["content_base64"], "wrong pass", ["settings"])

    def test_update_is_checksum_verified_before_staging(self):
        payload = b"signed later in production"
        encoded = base64.b64encode(payload).decode("ascii")
        manifest = {"version": "0.15.0-dev", "sha256": hashlib.sha256(payload).hexdigest(), "development_unsigned": True}
        staged = self.service.stage_update(manifest, encoded)
        self.assertEqual(staged["data"]["state"], "staged")
        manifest["sha256"] = "0" * 64
        with self.assertRaises(ApiError):
            self.service.stage_update(manifest, encoded)

    def test_recording_backup_restores_audio_sidecars_and_stages_database(self):
        archive = RecordingArchive(self.service.recordings_db, self.service.recordings_root)
        recording = RecordingService(
            archive,
            backend_factory=lambda: SyntheticCaptureBackend(seconds=0.08),
            minimum_free_bytes=1,
        )
        recording.start("shaer_ghibli_garden")
        item = recording.stop()
        relative_audio = Path(item["file_path"]).relative_to(self.service.recordings_root)
        backup = self.service.create_backup("archive passphrase", ["recordings"])["data"]
        archive.close()
        shutil.rmtree(self.service.recordings_root)
        self.service.recordings_db.unlink()
        restored = self.service.restore_backup(backup["content_base64"], "archive passphrase", ["recordings"])
        self.assertTrue(restored["data"]["reboot_required"])
        self.assertTrue((self.service.recordings_root / relative_audio).is_file())
        self.assertTrue(self.service.recordings_db.with_suffix(".db.restore-pending").is_file())


class CompanionHttpIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        try:
            with socket.socket() as listener:
                listener.bind(("127.0.0.1", 0))
                self.port = listener.getsockname()[1]
        except PermissionError:
            self.skipTest("This sandbox does not permit localhost sockets.")
        env = os.environ.copy()
        env["SHAER_CONFIG_DIR"] = str(Path(self.temporary.name) / "config")
        env["PYTHONPYCACHEPREFIX"] = str(Path(self.temporary.name) / "pycache")
        env["SHAER_RECORDING_TEST_MODE"] = "1"
        env["SHAER_RECORDING_MIN_FREE_BYTES"] = "1"
        self.process = subprocess.Popen(
            [sys.executable, str(OUTPUTS / "shaer_pi_os" / "server.py"), "--host", "127.0.0.1", "--port", str(self.port), "--allow-test-input"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
        )
        for _ in range(240):
            if self.process.poll() is not None:
                output = self.process.stdout.read() if self.process.stdout else ""
                self.fail(f"SHAeR server exited during startup:\n{output}")
            try:
                self.request("GET", "/api/v1/device/discovery")
                break
            except (urllib.error.URLError, ConnectionError):
                time.sleep(0.05)
        else:
            self.fail("SHAeR server did not start")

    def tearDown(self):
        self.process.terminate()
        try:
            self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
        if self.process.stdout:
            self.process.stdout.close()
        self.temporary.cleanup()

    def request(self, method, path, payload=None, token=None):
        body = json.dumps(payload).encode("utf-8") if payload is not None else None
        headers = {"Content-Type": "application/json"} if body else {}
        if token:
            headers["Authorization"] = f"Bearer {token}"
        request = urllib.request.Request(f"http://127.0.0.1:{self.port}{path}", data=body, headers=headers, method=method)
        with urllib.request.urlopen(request, timeout=3) as response:
            return json.loads(response.read())

    def request_bytes(self, method, path, token=None):
        headers = {"Authorization": f"Bearer {token}"} if token else {}
        request = urllib.request.Request(f"http://127.0.0.1:{self.port}{path}", headers=headers, method=method)
        with urllib.request.urlopen(request, timeout=3) as response:
            return response.headers, response.read()

    def physical_nonce(self):
        self.request("POST", "/api/debug/input", {"action": "select"})
        events = self.request("GET", "/api/events?after=0")["events"]
        return next(event["physical_nonce"] for event in reversed(events) if event.get("physical_nonce"))

    def test_full_pairing_and_authenticated_control_flow(self):
        discovery = self.request("GET", "/api/v1/device/discovery")
        self.assertEqual(discovery["data"]["protocol_version"], 1)
        pairing = self.request("POST", "/api/v1/pairing/start", {"device_name": "Integration phone"})["data"]
        self.request("POST", "/api/v1/pairing/approve", {"pairing_id": pairing["pairing_id"], "approved": True, "physical_nonce": self.physical_nonce()})
        token = self.request("GET", f"/api/v1/pairing/status?pairing_id={pairing['pairing_id']}")["data"]["token"]
        dashboard = self.request("GET", "/api/v1/dashboard", token=token)
        self.assertEqual(dashboard["data"]["firmware_version"], "0.16.0")
        settings = self.request("POST", "/api/v1/settings", {"display": {"brightness": 61}}, token)
        self.assertEqual(settings["data"]["display"]["brightness"], 61)
        control = self.request("POST", "/api/v1/playback/control", {"action": "play_pause"}, token)
        self.assertTrue(control["data"]["mirrored"])
        spotify = self.request("GET", "/api/v1/spotify/status", token=token)
        self.assertFalse(spotify["data"]["authenticated"])

    def test_recording_lifecycle_archive_metadata_and_download(self):
        pairing = self.request("POST", "/api/v1/pairing/start", {"device_name": "Recording test"})["data"]
        self.request("POST", "/api/v1/pairing/approve", {"pairing_id": pairing["pairing_id"], "approved": True, "physical_nonce": self.physical_nonce()})
        token = self.request("GET", f"/api/v1/pairing/status?pairing_id={pairing['pairing_id']}")["data"]["token"]
        self.assertEqual(self.request("POST", "/api/recording/control", {"action": "start", "theme": "shaer_bombay_ticket"})["data"]["state"], "recording")
        self.assertEqual(self.request("POST", "/api/recording/control", {"action": "pause"})["data"]["state"], "paused")
        self.assertEqual(self.request("POST", "/api/recording/control", {"action": "resume"})["data"]["state"], "recording")
        saved = self.request("POST", "/api/recording/control", {"action": "stop"})["data"]
        listing = self.request("GET", "/api/v1/recordings", token=token)["data"]
        self.assertEqual(listing["recordings"][0]["id"], saved["id"])
        updated = self.request("POST", f"/api/v1/recordings/{saved['id']}", {"title": "Pi field note", "favorite": True}, token)["data"]
        self.assertEqual(updated["title"], "Pi field note")
        headers, audio = self.request_bytes("GET", f"/api/v1/recordings/{saved['id']}/download", token)
        self.assertEqual(headers.get_content_type(), "audio/wav")
        self.assertGreater(len(audio), 44)
        inline_headers, inline_audio = self.request_bytes("GET", f"/api/recording/audio/{saved['id']}")
        self.assertEqual(inline_headers.get("Content-Disposition", "").split(";", 1)[0], "inline")
        self.assertEqual(inline_audio, audio)


if __name__ == "__main__":
    unittest.main()
