import base64
import io
import json
import os
import sqlite3
import stat
import sys
import tempfile
import threading
import zipfile
from pathlib import Path
from unittest import mock


OUTPUTS = Path(__file__).resolve().parents[2]
BACKEND = OUTPUTS / "shaer_backend"
for path in (OUTPUTS, BACKEND, OUTPUTS / "shaer_pi_os"):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from shaer_companion import ApiError, CompanionService  # noqa: E402
from shaer_companion.protocol import JsonStore  # noqa: E402
from server import ShaerHandler, consume_physical_capability, issue_physical_capability  # noqa: E402


class SecurityHardeningTests(__import__("unittest").TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name) / "outputs"
        self.root.mkdir()
        self.service = CompanionService(self.root, Path(self.temporary.name) / "config")

    def tearDown(self):
        self.temporary.cleanup()

    def test_physical_capability_is_one_use(self):
        nonce = issue_physical_capability()
        self.assertTrue(consume_physical_capability(nonce))
        self.assertFalse(consume_physical_capability(nonce))
        self.assertFalse(consume_physical_capability("unknown"))
        scoped = issue_physical_capability(["pairing-a"])
        self.assertFalse(consume_physical_capability(scoped, "pairing-b"))

    def test_static_server_exposes_ui_assets_but_not_python_or_docs(self):
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_base_dark/src/base-theme.css"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_base_light/src/base-theme.css"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_base_light/src/base-light.css"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_base_light/src/base-theme.js"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_dark_archive/src/dark-archive.js"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_companion/src/companion.js"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_companion/manifest.webmanifest"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_companion/service-worker.js"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_companion/icons/shaer-192.png"))
        self.assertTrue(ShaerHandler._static_path_allowed("/shaer_pi_os/hardware-bridge.js"))
        self.assertFalse(ShaerHandler._static_path_allowed("/shaer_pi_os/server.py"))
        self.assertFalse(ShaerHandler._static_path_allowed("/shaer_companion/protocol.py"))
        self.assertFalse(ShaerHandler._static_path_allowed("/docs/COMPANION_PROTOCOL.md"))
        self.assertFalse(ShaerHandler._static_path_allowed("/shaer_dark_archive/../shaer_pi_os/server.py"))

    def test_pairing_is_rate_limited_and_pending_sessions_are_capped(self):
        self.service.pairing.start("Phone 1", "10.0.0.1")
        with self.assertRaisesRegex(ApiError, "Wait before"):
            self.service.pairing.start("Phone 2", "10.0.0.1")
        self.service.pairing.start("Phone 2", "10.0.0.2")
        self.service.pairing.start("Phone 3", "10.0.0.3")
        with self.assertRaisesRegex(ApiError, "too many"):
            self.service.pairing.start("Phone 4", "10.0.0.4")

    def test_settings_capability_contracts_do_not_fabricate_hardware(self):
        capabilities = self.service.capabilities(power_actions=False)["data"]["capabilities"]
        self.assertEqual(capabilities["power"]["actions"]["state"], "permission_required")
        self.assertEqual(capabilities["power"]["battery"]["state"], "hardware_missing")
        self.assertEqual(capabilities["power"]["charging_current"]["state"], "hardware_missing")

        power = self.service.power()["data"]
        self.assertIsNone(power["battery_percent"])
        self.assertEqual(power["charge_cycles"]["state"], "hardware_missing")

        appearance = self.service.settings_domain("appearance")["data"]
        self.assertEqual(appearance["source_category"], "display")
        self.assertTrue(any(field["key"] == "theme" for field in appearance["fields"]))

    def test_json_store_mutation_does_not_lose_parallel_updates(self):
        store = JsonStore(Path(self.temporary.name) / "counter.json", {"count": 0})
        workers = []
        for _ in range(20):
            worker = threading.Thread(target=lambda: store.mutate(lambda data: data.__setitem__("count", int(data["count"]) + 1)))
            worker.start()
            workers.append(worker)
        for worker in workers:
            worker.join()
        self.assertEqual(store.load()["count"], 20)

    def test_audio_extension_without_matching_magic_is_rejected(self):
        with self.assertRaisesRegex(ApiError, "contents"):
            self.service.upload_track("not-music.mp3", base64.b64encode(b"plain text").decode("ascii"))

    def test_invalid_playlist_track_ids_do_not_create_or_modify_playlist(self):
        uploaded = self.service.upload_track("valid.mp3", base64.b64encode(b"ID3\x04\x00\x00\x00\x00\x00\x00").decode("ascii"))
        self.assertTrue(uploaded["ok"])
        track_id = self.service.music()["data"]["tracks"][0]["id"]
        with self.assertRaisesRegex(ApiError, "Unknown track"):
            self.service.create_playlist("Broken", [track_id, 999999])
        self.assertEqual(self.service.playlists()["data"]["playlists"], [])
        playlist_id = self.service.create_playlist("Good", [track_id])["data"]["playlist"]["id"]
        with self.assertRaisesRegex(ApiError, "Unknown track"):
            self.service.update_playlist(playlist_id, "Should Roll Back", [999999])
        current = self.service.playlist(playlist_id)["data"]
        self.assertEqual(current["playlist"]["name"], "Good")
        self.assertEqual([item["id"] for item in current["tracks"]], [track_id])

    def test_archive_symlink_and_extreme_ratio_are_rejected(self):
        symlink_buffer = io.BytesIO()
        with zipfile.ZipFile(symlink_buffer, "w") as archive:
            member = zipfile.ZipInfo("shaer_test/link")
            member.external_attr = (stat.S_IFLNK | 0o777) << 16
            archive.writestr(member, "../../outside")
        with zipfile.ZipFile(io.BytesIO(symlink_buffer.getvalue())) as archive:
            with self.assertRaisesRegex(ApiError, "link or special"):
                self.service._validate_archive(archive, 20, 1024 * 1024)

        ratio_buffer = io.BytesIO()
        with zipfile.ZipFile(ratio_buffer, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("huge.txt", b"0" * (2 * 1024 * 1024))
        with zipfile.ZipFile(io.BytesIO(ratio_buffer.getvalue())) as archive:
            with self.assertRaisesRegex(ApiError, "ratio"):
                self.service._validate_archive(archive, 20, 4 * 1024 * 1024)

    def test_corrupt_sqlite_restore_is_rejected_before_settings_change(self):
        self.service.update_settings({"display": {"brightness": 33}})
        archive_buffer = io.BytesIO()
        with zipfile.ZipFile(archive_buffer, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("manifest.json", json.dumps({"format": "shaer-backup-v1"}))
            archive.writestr("settings/settings.json", json.dumps({"display": {"brightness": 99}}))
            archive.writestr("library/library.db", b"not sqlite")
        encrypted = self.service._encrypt(archive_buffer.getvalue(), "correct horse")
        with self.assertRaisesRegex(ApiError, "SQLite"):
            self.service.restore_backup(base64.b64encode(encrypted).decode("ascii"), "correct horse", ["settings", "library"])
        self.assertEqual(self.service.settings()["data"]["display"]["brightness"], 33)

    def test_restore_commit_rolls_back_every_prior_swap_on_io_failure(self):
        root = Path(self.temporary.name) / "transaction"
        root.mkdir()
        first, second = root / "first", root / "second"
        first.write_text("old-first", encoding="utf-8")
        second.write_text("old-second", encoding="utf-8")
        staged_first, staged_second = root / "new-first", root / "new-second"
        staged_first.write_text("new-first", encoding="utf-8")
        staged_second.write_text("new-second", encoding="utf-8")
        real_replace = os.replace
        failed = False

        def fail_second(source, destination):
            nonlocal failed
            if not failed and Path(source) == staged_second and Path(destination) == second:
                failed = True
                raise OSError("simulated disk fault")
            return real_replace(source, destination)

        with mock.patch("shaer_companion.protocol.os.replace", side_effect=fail_second):
            with self.assertRaisesRegex(ApiError, "rolled back"):
                self.service._commit_restore([(first, staged_first), (second, staged_second)])
        self.assertEqual(first.read_text(encoding="utf-8"), "old-first")
        self.assertEqual(second.read_text(encoding="utf-8"), "old-second")

    def test_unsigned_theme_import_is_disabled(self):
        with self.assertRaisesRegex(ApiError, "disabled"):
            self.service.import_theme("theme.zip", base64.b64encode(b"not a zip").decode("ascii"))


if __name__ == "__main__":
    __import__("unittest").main()
