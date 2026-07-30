#!/usr/bin/env python3
"""SHAeR Pi OS static server with optional GPIO input bridge."""

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import secrets
import signal
import shutil
import socket
import subprocess
import sys
import threading
import time
import zipfile
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse


ROOT = Path(__file__).resolve().parents[1]
BACKEND_ROOT = ROOT / "shaer_backend"
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
if str(BACKEND_ROOT) not in sys.path:
    sys.path.insert(0, str(BACKEND_ROOT))

from shaer_companion import ApiError, CompanionService, PROTOCOL_VERSION  # noqa: E402
from shaer_archive import ArchiveError, MusicalArchive  # noqa: E402
from shaer_recording import RecordingArchive, RecordingError, RecordingService, SyntheticCaptureBackend  # noqa: E402
from shaer_hal import GpioInputController  # noqa: E402
from spotify_api import SpotifyApiError, SpotifyAuthError, SpotifyRuntime  # noqa: E402

EVENTS: list[dict[str, object]] = []
EVENT_LOCK = threading.Lock()
EVENT_CONDITION = threading.Condition(EVENT_LOCK)
NEXT_EVENT_ID = 1
METRICS: list[dict[str, object]] = []
METRICS_LOCK = threading.Lock()
GPIO_DEVICES: list[object] = []
EVENT_WAITERS = threading.BoundedSemaphore(4)
PHYSICAL_CAPABILITIES: dict[str, tuple[float, frozenset[str]]] = {}
PHYSICAL_CAPABILITIES_LOCK = threading.Lock()
PHYSICAL_CAPABILITY_TTL_S = 8.0
PENDING_PAIRING_IDS_PROVIDER = lambda: []
LOCAL_PLAYBACK_LOCK = threading.Lock()
LOCAL_PLAYBACK_STATE: dict[str, object] = {
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
}

UDP_DISCOVERY_PORT = 8776
UDP_DISCOVERY_REQUEST = b"SHAER_DISCOVER_V1"
SHAER_GITHUB_URL = "https://github.com/Rishikakaps/SHAeR"
MAX_FEEDBACK_REPORTS = 200


class UdpDiscoveryResponder:
    def __init__(self, companion: CompanionService, spotify: SpotifyRuntime, api_port: int):
        self.companion = companion
        self.spotify = spotify
        self.api_port = api_port
        self.stopping = threading.Event()
        self.thread: threading.Thread | None = None

    def start(self) -> None:
        self.thread = threading.Thread(target=self._serve, name="shaer-udp-discovery", daemon=True)
        self.thread.start()
        print(f"UDP discovery: listening on 0.0.0.0:{UDP_DISCOVERY_PORT}")

    def stop(self) -> None:
        self.stopping.set()
        if self.thread is not None:
            self.thread.join(timeout=2)

    def _serve(self) -> None:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                sock.bind(("0.0.0.0", UDP_DISCOVERY_PORT))
                sock.settimeout(0.5)
                while not self.stopping.is_set():
                    try:
                        payload, address = sock.recvfrom(256)
                    except socket.timeout:
                        continue
                    if payload.strip() != UDP_DISCOVERY_REQUEST:
                        continue
                    data = self.companion.discovery(self.spotify.status()).get("data", {})
                    if not isinstance(data, dict):
                        continue
                    response = {
                        "service": "shaer",
                        "deviceId": data.get("device_id", "shaer-local"),
                        "deviceName": data.get("device_name", "SHAeR"),
                        "host": self._local_host(sock, address[0]),
                        "port": self.api_port,
                        "apiVersion": str(data.get("protocol_version", PROTOCOL_VERSION)),
                    }
                    sock.sendto(json.dumps(response, separators=(",", ":")).encode("utf-8"), address)
        except OSError as exc:
            print(f"UDP discovery unavailable: {exc}")

    @staticmethod
    def _local_host(sock: socket.socket, destination: str) -> str:
        try:
            probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            try:
                probe.connect((destination, 1))
                return probe.getsockname()[0]
            finally:
                probe.close()
        except OSError:
            return sock.getsockname()[0]


def issue_physical_capability(pairing_ids: list[str] | None = None) -> str:
    token = secrets.token_urlsafe(24)
    now = time.monotonic()
    with PHYSICAL_CAPABILITIES_LOCK:
        for candidate, (expires_at, _pairing_ids) in list(PHYSICAL_CAPABILITIES.items()):
            if expires_at <= now:
                PHYSICAL_CAPABILITIES.pop(candidate, None)
        PHYSICAL_CAPABILITIES[token] = (now + PHYSICAL_CAPABILITY_TTL_S, frozenset(pairing_ids or []))
    return token


def consume_physical_capability(token: object, pairing_id: str | None = None) -> bool:
    if not isinstance(token, str) or not token:
        return False
    now = time.monotonic()
    with PHYSICAL_CAPABILITIES_LOCK:
        capability = PHYSICAL_CAPABILITIES.pop(token, None)
        for candidate, (expiry, _pairing_ids) in list(PHYSICAL_CAPABILITIES.items()):
            if expiry <= now:
                PHYSICAL_CAPABILITIES.pop(candidate, None)
    if capability is None:
        return False
    expires_at, pairing_ids = capability
    return expires_at > now and (pairing_id is None or pairing_id in pairing_ids)


def push_event(action: str, source: str = "server") -> None:
    global NEXT_EVENT_ID
    allowed = {"left", "right", "select", "back", "home", "menu", "long_select", "toggle_input_mode", "volume_up", "volume_down"}
    if action not in allowed and not action.startswith("theme:shaer_"):
        return
    with EVENT_CONDITION:
        event: dict[str, object] = {
            "id": NEXT_EVENT_ID,
            "action": action,
            "source": source,
            "time": time.time(),
        }
        if source in {"gpio", "debug-api"} and action in {"select", "long_select"}:
            try:
                pairing_ids = list(PENDING_PAIRING_IDS_PROVIDER())
            except Exception:
                pairing_ids = []
            event["physical_nonce"] = issue_physical_capability(pairing_ids)
        EVENTS.append(event)
        NEXT_EVENT_ID += 1
        del EVENTS[:-80]
        EVENT_CONDITION.notify_all()


def events_after(after: int, timeout: float = 0.25) -> list[dict[str, object]]:
    deadline = time.time() + timeout
    with EVENT_CONDITION:
        while not any(int(event["id"]) > after for event in EVENTS):
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            EVENT_CONDITION.wait(remaining)
        return [event for event in EVENTS if int(event["id"]) > after]


class ShaerHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, theme: str, spotify: SpotifyRuntime, companion: CompanionService, recording: RecordingService, archive: MusicalArchive, allow_power: bool, allow_test_input: bool, **kwargs):
        self.theme = theme
        self.spotify = spotify
        self.companion = companion
        self.recording = recording
        self.archive = archive
        self.allow_power = allow_power
        self.allow_test_input = allow_test_input
        self._suppress_log = False
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def log_message(self, fmt: str, *args) -> None:
        if self._suppress_log:
            return
        sys.stdout.write("[%s] %s\n" % (self.log_date_time_string(), fmt % args))

    def setup(self) -> None:
        super().setup()
        self.connection.settimeout(10)

    def end_headers(self) -> None:
        self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob: https:; media-src 'self' blob:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Permissions-Policy", "camera=(), geolocation=(), microphone=()")
        origin = self.headers.get("Origin", "")
        allowed_origins = {"https://localhost", "http://localhost", "capacitor://localhost"}
        configured_origins = {item.strip() for item in os.environ.get("SHAER_COMPANION_ORIGINS", "").split(",") if item.strip()}
        if origin in allowed_origins | configured_origins and urlparse(self.path).path.startswith("/api/v1/"):
            self.send_header("Access-Control-Allow-Origin", origin)
            self.send_header("Access-Control-Allow-Headers", "Authorization, Content-Type")
            self.send_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS")
            self.send_header("Access-Control-Allow-Private-Network", "true")
            self.send_header("Vary", "Origin")
            self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        else:
            self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        super().end_headers()

    def do_OPTIONS(self) -> None:
        if not urlparse(self.path).path.startswith("/api/v1/"):
            self.send_error(404)
            return
        self.send_response(204)
        self.end_headers()

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/favicon.ico":
            self._suppress_log = True
            self.send_response(204)
            self.end_headers()
            return
        if parsed.path == "/":
            settings = self.companion.settings().get("data", {})
            display = settings.get("display", {}) if isinstance(settings, dict) else {}
            active_theme = display.get("theme", self.theme) if isinstance(display, dict) else self.theme
            self.send_response(302)
            self.send_header("Location", f"/{active_theme}/?mode=device")
            self.end_headers()
            return
        if parsed.path == "/companion":
            self.send_response(302)
            self.send_header("Location", "/shaer_companion/")
            self.end_headers()
            return
        if parsed.path.startswith("/api/recording/"):
            if parsed.path == "/api/recording/status":
                self._suppress_log = True
            self._recording_get(parsed.path, parse_qs(parsed.query))
            return
        if parsed.path == "/api/archive" or parsed.path.startswith("/api/archive/"):
            self._archive_get(parsed.path, parse_qs(parsed.query))
            return
        if parsed.path.startswith("/marginalia/"):
            self._send_marginalia_file(parsed.path)
            return
        if parsed.path.startswith("/api/v1/"):
            if parsed.path in {"/api/v1/dashboard", "/api/v1/pairing/pending", "/api/v1/pairing/status", "/api/v1/pairing/state"}:
                self._suppress_log = True
            self._companion_get(parsed.path, parse_qs(parsed.query))
            return
        if parsed.path == "/api/onboarding/download-qr.svg":
            self._suppress_log = True
            self._send_download_qr_svg()
            return
        if parsed.path == "/api/events":
            self._suppress_log = True
            try:
                self._require_local_device()
                query = parse_qs(parsed.query)
                after = max(0, int(query.get("after", ["0"])[0] or 0))
                if not EVENT_WAITERS.acquire(blocking=False):
                    raise ApiError("too_many_event_waiters", "Too many event polling requests.", 429)
                try:
                    self.send_json({"events": events_after(after)})
                finally:
                    EVENT_WAITERS.release()
            except (ApiError, ValueError) as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/push":
            self.send_json({"ok": False, "error": {"code": "method_not_allowed", "message": "Test input is POST-only and disabled by default."}}, status=405)
            return
        if parsed.path == "/api/system/metrics":
            self._suppress_log = True
            try:
                self._require_local_device()
                with METRICS_LOCK:
                    latest = METRICS[-1] if METRICS else {}
                self.send_json({"latest": latest, "samples": len(METRICS)})
            except ApiError as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/system/capabilities":
            self._suppress_log = True
            try:
                self._require_local_device()
                self.send_json({
                    "capabilities": {
                        "power": self.allow_power,
                        "recording": True,
                        "spotify": True,
                        "hardware_input": bool(GPIO_DEVICES),
                        "state": self.companion.capabilities(
                            power_actions=self.allow_power,
                            hardware_input=bool(GPIO_DEVICES),
                            recording=True,
                        ).get("data", {}).get("capabilities", {}),
                    }
                })
            except ApiError as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/music/tracks":
            try:
                self._require_local_device()
                query = parse_qs(parsed.query)
                result = self.companion.music((query.get("q") or [""])[0])
                data = result.get("data", {}) if isinstance(result, dict) else {}
                tracks = data.get("tracks", []) if isinstance(data, dict) else []
                limit = min(200, max(1, int((query.get("limit") or ["50"])[0])))
                self.send_json({"tracks": tracks[:limit] if isinstance(tracks, list) else []})
            except (ApiError, ValueError, OSError, RuntimeError) as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/music/playlists":
            try:
                self._require_local_device()
                result = self.companion.playlists()
                data = result.get("data", {}) if isinstance(result, dict) else {}
                self.send_json({"playlists": data.get("playlists", []) if isinstance(data, dict) else []})
            except (ApiError, ValueError, OSError, RuntimeError) as exc:
                self._send_companion_error(exc)
            return
        if parsed.path.startswith("/api/music/playlists/") and parsed.path.endswith("/tracks"):
            try:
                self._require_local_device()
                parts = parsed.path.strip("/").split("/")
                playlist_id = int(parts[3])
                result = self.companion.playlist(playlist_id)
                data = result.get("data", {}) if isinstance(result, dict) else {}
                self.send_json({
                    "playlist": data.get("playlist", {}) if isinstance(data, dict) else {},
                    "tracks": data.get("tracks", []) if isinstance(data, dict) else [],
                })
            except (ApiError, ValueError, OSError, RuntimeError) as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/music/playback":
            try:
                self._require_local_device()
                self.send_json(self._local_playback_status())
            except ApiError as exc:
                self._send_companion_error(exc)
            return
        if parsed.path.startswith("/api/spotify/"):
            if parsed.path in {"/api/spotify/status", "/api/spotify/playback"}:
                self._suppress_log = True
            self._spotify_get(parsed.path, parse_qs(parsed.query))
            return
        if not self._static_path_allowed(parsed.path):
            self.send_error(404)
            return
        super().do_GET()

    def do_HEAD(self) -> None:
        parsed = urlparse(self.path)
        if not self._static_path_allowed(parsed.path):
            self.send_error(404)
            return
        super().do_HEAD()

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path.startswith("/api/recording/"):
            self._recording_post(parsed.path)
            return
        if parsed.path == "/api/archive/entries" or "/marginalia" in parsed.path:
            self._archive_post(parsed.path)
            return
        if parsed.path.startswith("/api/v1/"):
            self._companion_post(parsed.path)
            return
        if parsed.path == "/api/debug/input":
            try:
                self._require_local_device()
                if not self.allow_test_input:
                    raise ApiError("test_input_disabled", "Debug input is disabled in this runtime.", 403)
                payload = self._read_json(1024)
                action = str(payload.get("action") or "")
                allowed = {"left", "right", "select", "back", "home", "menu", "long_select", "toggle_input_mode", "volume_up", "volume_down"}
                if action not in allowed:
                    raise ApiError("invalid_input", "Unknown debug input action.")
                push_event(action, "debug-api")
                self.send_json({"ok": True, "action": action})
            except (ApiError, ValueError, OSError) as exc:
                self._send_companion_error(exc)
            return
        if parsed.path == "/api/music/playback":
            try:
                self._require_local_device()
                payload = self._read_json(4096)
                self.send_json(self._local_playback_post(payload))
            except (ApiError, ValueError, OSError, RuntimeError) as exc:
                self._send_companion_error(exc)
            return
        if not parsed.path.startswith("/api/spotify/") and not parsed.path.startswith("/api/system/"):
            self.send_error(404)
            return
        try:
            self._require_local_device()
            payload = self._read_json(4096 if parsed.path.startswith("/api/system/") else 65536)
            result = self._system_post(parsed.path, payload) if parsed.path.startswith("/api/system/") else self._spotify_post(parsed.path, payload)
            self.send_json(result)
        except ApiError as exc:
            self._send_companion_error(exc)
        except (SpotifyAuthError, SpotifyApiError, ValueError, OSError, RuntimeError) as exc:
            self.send_json({"ok": False, "error": str(exc)}, status=self._spotify_error_status(exc))

    def do_DELETE(self) -> None:
        parsed = urlparse(self.path)
        if not parsed.path.startswith("/api/v1/"):
            self.send_error(404)
            return
        try:
            self._companion_authenticate()
            parts = parsed.path.strip("/").split("/")
            if len(parts) == 5 and parts[2] == "music" and parts[3] == "tracks":
                result = self.companion.delete_track(int(parts[4]))
            elif len(parts) == 5 and parts[2] == "music" and parts[3] == "playlists":
                result = self.companion.delete_playlist(int(parts[4]))
            elif len(parts) == 4 and parts[2] == "themes":
                result = self.companion.delete_theme(parts[3])
            elif len(parts) == 5 and parts[2] == "pairing" and parts[3] == "trusted":
                result = self.companion.envelope({"forgotten": self.companion.pairing.forget(parts[4])})
            elif len(parts) == 4 and parts[2] == "recordings":
                recording_id = int(parts[3])
                self.recording.archive.delete(recording_id)
                result = self.companion.envelope({"id": recording_id, "deleted": True})
            else:
                raise ApiError("not_found", "Unknown companion endpoint.", 404)
            self.send_json(result)
        except (ApiError, RecordingError, ValueError) as exc:
            self._send_companion_error(exc)

    def _companion_get(self, path: str, query: dict[str, list[str]]) -> None:
        try:
            if path == "/api/v1/device/discovery":
                result = self.companion.discovery(self._spotify_status())
            elif path == "/api/v1/pairing/state":
                devices = self.companion.pairing.trusted_devices()
                result = self.companion.envelope({
                    "trusted_count": len(devices),
                    "paired": bool(devices),
                    "download_url": SHAER_GITHUB_URL,
                })
            elif path == "/api/v1/pairing/status":
                result = self.companion.envelope(self.companion.pairing.status((query.get("pairing_id") or [""])[0]))
            elif path == "/api/v1/pairing/pending":
                self._require_local_device()
                result = self.companion.envelope({"requests": self.companion.pairing.pending_for_device()})
            else:
                self._companion_authenticate()
                if path == "/api/v1/dashboard":
                    result = self.companion.dashboard(self._spotify_status(), self._playback_status())
                elif path == "/api/v1/device":
                    result = self.companion.device(self._spotify_status())
                elif path == "/api/v1/device/capabilities":
                    result = self.companion.capabilities(power_actions=self.allow_power, hardware_input=bool(GPIO_DEVICES), recording=True)
                elif path == "/api/v1/storage":
                    result = self.companion.storage()
                elif path == "/api/v1/branding":
                    result = self.companion.envelope(self._branding())
                elif path == "/api/v1/network/wifi":
                    result = self.companion.envelope(self._wifi_status())
                elif path == "/api/v1/network/wifi/scan":
                    result = self.companion.envelope(self._wifi_scan())
                elif path == "/api/v1/bluetooth":
                    result = self.companion.envelope(self._bluetooth_status())
                elif path == "/api/v1/bluetooth/scan":
                    result = self.companion.envelope(self._bluetooth_scan())
                elif path == "/api/v1/power":
                    result = self.companion.power()
                elif path == "/api/v1/settings":
                    result = self.companion.settings()
                elif path.startswith("/api/v1/settings/"):
                    result = self.companion.settings_domain(path.strip("/").split("/", 3)[3])
                elif path == "/api/v1/themes":
                    result = self.companion.themes()
                elif path.startswith("/api/v1/themes/") and path.endswith("/export"):
                    result = self.companion.export_theme(path.split("/")[4])
                elif path == "/api/v1/diagnostics":
                    result = self.companion.diagnostics()
                elif path == "/api/v1/feedback":
                    result = self.companion.envelope({"reports": self._feedback_reports()})
                elif path == "/api/v1/developer/dashboard":
                    result = self.companion.envelope(self._developer_dashboard())
                elif path == "/api/v1/music/tracks":
                    result = self.companion.music((query.get("q") or [""])[0])
                elif path == "/api/v1/music/playlists":
                    result = self.companion.playlists()
                elif path == "/api/v1/archive":
                    result = self.companion.envelope({"entries": self.archive.list((query.get("q") or [""])[0], int((query.get("limit") or ["200"])[0]))})
                elif path.startswith("/api/v1/archive/"):
                    parts = path.strip("/").split("/")
                    entry_id = int(parts[3])
                    if len(parts) == 5 and parts[4] == "marginalia":
                        result = self.companion.envelope({"archive_entry_id": entry_id, "pages": self.archive.marginalia(entry_id)})
                    else:
                        result = self.companion.envelope(self.archive.get(entry_id))
                elif path.startswith("/api/v1/music/playlists/"):
                    parts = path.strip("/").split("/")
                    playlist_id = int(parts[4])
                    result = self.companion.export_playlist(playlist_id) if len(parts) == 6 and parts[5] == "export" else self.companion.playlist(playlist_id)
                elif path == "/api/v1/pairing/trusted":
                    result = self.companion.envelope({"devices": self.companion.pairing.trusted_devices()})
                elif path == "/api/v1/spotify/status":
                    result = self.companion.envelope(self.spotify.status())
                elif path == "/api/v1/spotify/profile":
                    result = self.companion.envelope(self.spotify.profile())
                elif path == "/api/v1/spotify/playback":
                    result = self.companion.envelope(self.spotify.playback())
                elif path == "/api/v1/spotify/queue":
                    result = self.companion.envelope(self.spotify.queue())
                elif path.startswith("/api/v1/spotify/library/"):
                    result = self.companion.envelope(self.spotify.library(path.rsplit("/", 1)[-1], query))
                elif path == "/api/v1/spotify/search":
                    result = self.companion.envelope(self.spotify.search(query))
                elif path == "/api/v1/spotify/connect/status":
                    result = self.companion.envelope(self.spotify.connect_status())
                elif path == "/api/v1/updates/status":
                    result = self.companion.update_status()
                elif path == "/api/v1/recordings":
                    result = self.companion.envelope(self._recording_listing(query))
                elif path.startswith("/api/v1/recordings/"):
                    parts = path.strip("/").split("/")
                    recording_id = int(parts[3])
                    if len(parts) == 5 and parts[4] == "download":
                        self._send_recording_file(recording_id)
                        return
                    result = self.companion.envelope(self.recording.archive.get(recording_id))
                else:
                    raise ApiError("not_found", "Unknown companion endpoint.", 404)
            self.send_json(result)
        except (ApiError, ArchiveError, RecordingError, ValueError, OSError, RuntimeError) as exc:
            self._send_companion_error(exc)

    def _send_download_qr_svg(self) -> None:
        try:
            import qrcode  # type: ignore
            import qrcode.image.svg  # type: ignore

            image = qrcode.make(
                SHAER_GITHUB_URL,
                image_factory=qrcode.image.svg.SvgPathImage,
                box_size=8,
                border=2,
            )
            stream = io.BytesIO()
            image.save(stream)
            payload = stream.getvalue()
        except Exception:
            payload = (
                "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 280 280\">"
                "<rect width=\"280\" height=\"280\" fill=\"#fff\"/>"
                "<text x=\"140\" y=\"122\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"18\" fill=\"#111\">SHAeR</text>"
                "<text x=\"140\" y=\"150\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"10\" fill=\"#111\">github.com/Rishikakaps/SHAeR</text>"
                "</svg>"
            ).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "image/svg+xml; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def _companion_post(self, path: str) -> None:
        try:
            payload = self._read_json(90 * 1024 * 1024)
            if path == "/api/v1/pairing/start":
                result = self.companion.envelope(self.companion.pairing.start(str(payload.get("device_name") or ""), self.client_address[0]))
            elif path == "/api/v1/pairing/approve":
                self._require_local_device()
                pairing_id = str(payload.get("pairing_id") or "")
                self._require_physical_capability(payload, pairing_id)
                self.companion.pairing.approve(pairing_id, bool(payload.get("approved")))
                result = self.companion.envelope({"approved": bool(payload.get("approved"))})
            else:
                self._companion_authenticate()
                if path == "/api/v1/settings":
                    result = self.companion.update_settings(payload)
                elif path == "/api/v1/themes/active":
                    theme_id = str(payload.get("theme_id") or "")
                    result = self.companion.set_theme(theme_id)
                    push_event(f"theme:{theme_id}", "companion")
                elif path == "/api/v1/themes/import":
                    raise ApiError("theme_import_disabled", "Unsigned theme installation is disabled on this build.", 403)
                elif path == "/api/v1/diagnostics/run":
                    result = self.companion.run_diagnostic(str(payload.get("name") or ""), bool(payload.get("hardware")))
                elif path == "/api/v1/feedback":
                    result = self.companion.envelope(self._create_feedback_report(payload))
                elif path == "/api/v1/developer/releases":
                    result = self.companion.envelope(self._create_developer_release(payload))
                elif path == "/api/v1/music/upload":
                    result = self.companion.upload_track(str(payload.get("filename") or ""), str(payload.get("content_base64") or ""))
                elif path == "/api/v1/branding/app-icon":
                    result = self.companion.envelope(self._update_app_icon(payload))
                elif path == "/api/v1/archive/entries":
                    result = self.companion.envelope(self.archive.create_or_get(payload))
                elif path.startswith("/api/v1/archive/") and path.endswith("/marginalia"):
                    entry_id = int(path.strip("/").split("/")[3])
                    import base64
                    png = base64.b64decode(str(payload.get("image_base64") or ""), validate=True)
                    result = self.companion.envelope(self.archive.add_marginalia(entry_id, png, int(payload.get("playback_position_ms") or 0), str(payload.get("theme_id") or self.theme)))
                elif path == "/api/v1/music/playlists":
                    ids = payload.get("track_ids", [])
                    result = self.companion.create_playlist(str(payload.get("name") or ""), list(ids) if isinstance(ids, list) else [])
                elif path == "/api/v1/music/playlists/import":
                    result = self.companion.import_playlist(payload)
                elif path.startswith("/api/v1/music/playlists/"):
                    playlist_id = int(path.strip("/").split("/")[4])
                    ids = payload.get("track_ids")
                    result = self.companion.update_playlist(playlist_id, str(payload["name"]) if "name" in payload else None, list(ids) if isinstance(ids, list) else None)
                elif path.startswith("/api/v1/music/tracks/"):
                    result = self.companion.update_track(int(path.rsplit("/", 1)[-1]), payload)
                elif path == "/api/v1/playback/control":
                    result = self._companion_control(str(payload.get("action") or ""))
                elif path == "/api/v1/spotify/control":
                    result = self.companion.envelope(self._spotify_post("/api/spotify/control", payload))
                elif path == "/api/v1/spotify/logout":
                    result = self.companion.envelope(self._spotify_post("/api/spotify/logout", payload))
                elif path == "/api/v1/spotify/connect/transfer":
                    result = self.companion.envelope(self._spotify_post("/api/spotify/connect/transfer", payload))
                elif path == "/api/v1/backup/create":
                    include = payload.get("include")
                    result = self.companion.create_backup(str(payload.get("passphrase") or ""), list(include) if isinstance(include, list) else None)
                elif path == "/api/v1/backup/restore":
                    include = payload.get("include")
                    result = self.companion.restore_backup(str(payload.get("content_base64") or ""), str(payload.get("passphrase") or ""), list(include) if isinstance(include, list) else None)
                elif path == "/api/v1/updates/stage":
                    manifest = payload.get("manifest")
                    if not isinstance(manifest, dict):
                        raise ApiError("invalid_update", "Update manifest is required.")
                    result = self.companion.stage_update(manifest, str(payload.get("content_base64") or ""))
                elif path == "/api/v1/updates/install":
                    result = self.companion.install_update(False)
                elif path == "/api/v1/updates/rollback":
                    result = self.companion.install_update(True)
                elif path.startswith("/api/v1/recordings/"):
                    recording_id = int(path.strip("/").split("/")[3])
                    action = str(payload.get("action") or "update")
                    if action == "duplicate":
                        item = self.recording.archive.duplicate(recording_id)
                    elif action == "move":
                        item = self.recording.archive.move(recording_id, str(payload.get("folder") or ""))
                    else:
                        item = self.recording.archive.update(recording_id, payload)
                    result = self.companion.envelope(item)
                else:
                    raise ApiError("not_found", "Unknown companion endpoint.", 404)
            self.send_json(result)
        except (ApiError, ArchiveError, RecordingError, ValueError, OSError, RuntimeError, zipfile.BadZipFile) as exc:
            self._send_companion_error(exc)

    def _feedback_path(self) -> Path:
        return self.companion.config_dir / "feedback-reports.json"

    def _developer_releases_path(self) -> Path:
        return self.companion.config_dir / "developer-releases.json"

    @staticmethod
    def _load_json_list(path: Path) -> list[dict[str, object]]:
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            return []
        except (OSError, json.JSONDecodeError):
            return []
        return [item for item in payload if isinstance(item, dict)] if isinstance(payload, list) else []

    @staticmethod
    def _save_json_list(path: Path, items: list[dict[str, object]]) -> None:
        path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        temporary = path.with_suffix(path.suffix + ".tmp")
        temporary.write_text(json.dumps(items, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(temporary, path)
        path.chmod(0o600)

    def _feedback_reports(self) -> list[dict[str, object]]:
        return self._load_json_list(self._feedback_path())

    def _create_feedback_report(self, payload: dict[str, object]) -> dict[str, object]:
        message = str(payload.get("message") or "").strip()
        if len(message) < 3:
            raise ApiError("feedback_message_required", "Tell SHAeR what happened before sending feedback.")
        context = payload.get("context") if isinstance(payload.get("context"), dict) else {}
        report = {
            "id": f"fb-{int(time.time())}-{secrets.token_hex(4)}",
            "created_at": int(time.time()),
            "message": message[:2000],
            "severity": str(payload.get("severity") or "note")[:24],
            "context": context,
            "status": "new",
        }
        reports = self._feedback_reports()
        reports.insert(0, report)
        self._save_json_list(self._feedback_path(), reports[:MAX_FEEDBACK_REPORTS])
        return report

    def _developer_dashboard(self) -> dict[str, object]:
        releases = self._load_json_list(self._developer_releases_path())
        return {
            "feedback": self._feedback_reports(),
            "releases": releases,
            "device": self.companion.discovery(self._spotify_status()).get("data", {}),
            "updates": self.companion.update_status().get("data", {}),
        }

    def _create_developer_release(self, payload: dict[str, object]) -> dict[str, object]:
        version = str(payload.get("version") or "").strip()
        kind = str(payload.get("kind") or "app").strip()[:24]
        title = str(payload.get("title") or "I learned something new. Ready to update?").strip()
        notes = str(payload.get("notes") or "").strip()
        url = str(payload.get("url") or "").strip()
        if not version:
            raise ApiError("release_version_required", "Version is required.")
        release = {
            "id": f"rel-{int(time.time())}-{secrets.token_hex(4)}",
            "created_at": int(time.time()),
            "version": version[:64],
            "kind": kind,
            "title": title[:160],
            "notes": notes[:2000],
            "url": url[:1000],
            "status": "ready",
        }
        releases = self._load_json_list(self._developer_releases_path())
        releases.insert(0, release)
        self._save_json_list(self._developer_releases_path(), releases[:80])
        return release

    def _recording_get(self, path: str, query: dict[str, list[str]]) -> None:
        try:
            self._require_local_device()
            if path == "/api/recording/status":
                result = {"ok": True, **self.recording.status()}
            elif path == "/api/recording/library":
                result = {"ok": True, **self._recording_listing(query)}
            elif path.startswith("/api/recording/audio/"):
                recording_id = int(path.rsplit("/", 1)[-1])
                self._send_recording_file(recording_id, download=False)
                return
            else:
                raise RecordingError("not_found", "Unknown recording endpoint.")
            self.send_json(result)
        except (ApiError, RecordingError, ValueError, OSError) as exc:
            self._send_recording_error(exc)

    def _recording_post(self, path: str) -> None:
        try:
            self._require_local_device()
            payload = self._read_json(65536)
            if path != "/api/recording/control":
                raise RecordingError("not_found", "Unknown recording endpoint.")
            action = str(payload.get("action") or "")
            if action == "start":
                playback = self._playback_status()
                playback_active = str(playback.get("status") or "").lower() in {"playing", "buffering"}
                result = self.recording.start(str(payload.get("theme") or self.theme), playback_active)
            elif action == "pause":
                result = self.recording.pause()
            elif action == "resume":
                result = self.recording.resume()
            elif action == "stop":
                result = self.recording.stop("user")
            elif action == "cancel":
                result = self.recording.cancel()
            else:
                raise RecordingError("invalid_recording_action", "Unknown recording control action.")
            self.send_json({"ok": True, "data": result})
        except (ApiError, RecordingError, ValueError, OSError, RuntimeError) as exc:
            self._send_recording_error(exc)

    def _recording_listing(self, query: dict[str, list[str]]) -> dict[str, object]:
        favorite_raw = (query.get("favorite") or [""])[0]
        favorite = None if favorite_raw == "" else favorite_raw.lower() in {"1", "true", "yes"}
        year_raw = (query.get("year") or [""])[0]
        month_raw = (query.get("month") or [""])[0]
        recordings = self.recording.archive.list(
            query=(query.get("q") or [""])[0],
            favorite=favorite,
            year=int(year_raw) if year_raw else None,
            month=int(month_raw) if month_raw else None,
            minimum_duration_ms=int((query.get("duration_min_ms") or ["0"])[0] or 0) if "duration_min_ms" in query else None,
            maximum_duration_ms=int((query.get("duration_max_ms") or ["0"])[0] or 0) if "duration_max_ms" in query else None,
            limit=int((query.get("limit") or ["200"])[0] or 200),
        )
        return {"recordings": recordings, "storage": self.recording.archive.storage(), "active": self.recording.status()}

    def _archive_get(self, path: str, query: dict[str, list[str]]) -> None:
        try:
            if path == "/api/archive":
                result = {"entries": self.archive.list((query.get("q") or [""])[0], int((query.get("limit") or ["200"])[0]))}
            else:
                parts = path.strip("/").split("/")
                entry_id = int(parts[2])
                if len(parts) == 4 and parts[3] == "marginalia":
                    result = {"archive_entry_id": entry_id, "pages": self.archive.marginalia(entry_id)}
                else:
                    result = self.archive.get(entry_id)
            self.send_json({"ok": True, **result} if isinstance(result, dict) else {"data": result})
        except (ArchiveError, ValueError) as exc:
            self._send_archive_error(exc)

    def _archive_post(self, path: str) -> None:
        try:
            if path == "/api/archive/entries":
                result = self.archive.create_or_get(self._read_json(65536))
            else:
                parts = path.strip("/").split("/")
                if len(parts) != 4 or parts[0:2] != ["api", "archive"] or parts[3] != "marginalia":
                    raise ArchiveError("not_found", "Unknown archive endpoint.", 404)
                entry_id = int(parts[2])
                content_type = self.headers.get("Content-Type", "").split(";", 1)[0].lower()
                if content_type == "image/png":
                    length = int(self.headers.get("Content-Length", "0") or 0)
                    if length <= 0 or length > 4 * 1024 * 1024:
                        raise ArchiveError("invalid_marginalia_upload", "PNG upload size is invalid.")
                    png = self.rfile.read(length)
                    position = int(self.headers.get("X-SHAeR-Playback-Position-Ms", "0") or 0)
                    theme = self.headers.get("X-SHAeR-Theme-Id", self.theme)
                else:
                    payload = self._read_json(5 * 1024 * 1024)
                    import base64
                    png = base64.b64decode(str(payload.get("image_base64") or ""), validate=True)
                    position = int(payload.get("playback_position_ms") or 0)
                    theme = str(payload.get("theme_id") or self.theme)
                result = self.archive.add_marginalia(entry_id, png, position, theme)
            self.send_json({"ok": True, "data": result})
        except (ArchiveError, ValueError, OSError) as exc:
            self._send_archive_error(exc)

    def _send_marginalia_file(self, path: str) -> None:
        try:
            relative = unquote(path).removeprefix("/marginalia/")
            target = (self.archive.root / relative).resolve()
            if not self.archive._within(target, self.archive.root) or target.suffix.lower() != ".png" or not target.exists():
                raise ArchiveError("marginalia_not_found", "Marginalia image does not exist.", 404)
            body = target.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Cache-Control", "private, no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except (ArchiveError, OSError) as exc:
            self._send_archive_error(exc)

    def _send_archive_error(self, exc: Exception) -> None:
        if isinstance(exc, ArchiveError):
            self.send_json({"ok": False, "error": {"code": exc.code, "message": str(exc)}}, status=exc.status)
        else:
            self.send_json({"ok": False, "error": {"code": "archive_error", "message": str(exc)}}, status=400)

    def _send_recording_file(self, recording_id: int, download: bool = True) -> None:
        item = self.recording.archive.get(recording_id)
        path = Path(str(item["file_path"]))
        if not path.exists() or not self.recording.archive._within(path, self.recording.archive.root):
            raise RecordingError("recording_file_missing", "Recording audio file is missing.")
        size = path.stat().st_size
        start = 0
        end = max(0, size - 1)
        range_header = self.headers.get("Range", "")
        requested_range = range_header.startswith("bytes=")
        if requested_range:
            try:
                start_text, end_text = range_header[6:].split("-", 1)
                if not start_text and end_text:
                    suffix = int(end_text)
                    start = max(0, size - suffix)
                    end = size - 1
                else:
                    start = int(start_text) if start_text else 0
                    end = min(size - 1, int(end_text)) if end_text else size - 1
                if start < 0 or end < start or start >= size:
                    raise ValueError
            except ValueError:
                self.send_response(416)
                self.send_header("Content-Range", f"bytes */{size}")
                self.end_headers()
                return
        partial = requested_range
        self.send_response(206 if partial else 200)
        self.send_header("Content-Type", "audio/wav")
        disposition = "attachment" if download else "inline"
        self.send_header("Content-Disposition", f'{disposition}; filename="{path.name}"')
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Length", str(end - start + 1))
        if partial:
            self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
        self.send_header("Cache-Control", "private, no-store")
        self.end_headers()
        with path.open("rb") as source:
            source.seek(start)
            remaining = end - start + 1
            while remaining:
                chunk = source.read(min(1024 * 1024, remaining))
                if not chunk:
                    break
                self.wfile.write(chunk)
                remaining -= len(chunk)

    def _send_recording_error(self, exc: Exception) -> None:
        if isinstance(exc, ApiError):
            self.send_json({"ok": False, "error": {"code": exc.code, "message": str(exc)}}, status=exc.status)
        elif isinstance(exc, RecordingError):
            status = 404 if exc.code in {"recording_not_found", "not_found"} else 409 if exc.code in {"recording_busy", "audio_mode_conflict"} else 400
            self.send_json({"ok": False, "error": {"code": exc.code, "message": str(exc)}}, status=status)
        else:
            self.send_json({"ok": False, "error": {"code": "recording_error", "message": str(exc)}}, status=400)

    def _companion_control(self, action: str) -> dict[str, object]:
        actions = {"play_pause": "select", "next": "right", "previous": "left", "volume_up": "volume_up", "volume_down": "volume_down"}
        event = actions.get(action)
        if not event:
            raise ApiError("invalid_control", "Unknown mirrored playback control.")
        push_event(event, "companion")
        return self.companion.envelope({"action": action, "mirrored": True})

    def _branding(self) -> dict[str, object]:
        icon = ROOT / "shaer_companion" / "icons" / "custom-app-icon.png"
        return {
            "app_name": "SHAeR",
            "icon_url": "/shaer_companion/icons/custom-app-icon.png" if icon.exists() else "/shaer_companion/icons/shaer-512.png",
            "custom_icon": icon.exists(),
            "updated_at": icon.stat().st_mtime if icon.exists() else None,
            "installed_icon_note": "Launcher icons update after rebuilding and reinstalling the phone or desktop app."
        }

    def _update_app_icon(self, payload: dict[str, object]) -> dict[str, object]:
        mime = str(payload.get("mime_type") or "image/png").lower()
        if mime != "image/png":
            raise ApiError("unsupported_icon_type", "App icon upload currently accepts PNG images only.", 415)
        encoded = str(payload.get("content_base64") or "")
        try:
            image = base64.b64decode(encoded, validate=True)
        except ValueError:
            raise ApiError("invalid_icon", "Icon image is not valid base64 PNG data.")
        if len(image) < 32 or len(image) > 4 * 1024 * 1024:
            raise ApiError("invalid_icon_size", "Icon image must be between 32 bytes and 4 MB.")
        if not image.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ApiError("invalid_icon_type", "Icon image must be a PNG file.")
        targets = [
            ROOT / "shaer_companion" / "icons" / "custom-app-icon.png",
            ROOT / "shaer_companion" / "dist" / "icons" / "custom-app-icon.png",
        ]
        for target in targets:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(image)
        return self._branding()

    @staticmethod
    def _run_probe(command: list[str], timeout_s: float = 4.0) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout_s,
            check=False,
        )

    @staticmethod
    def _command_available(command: str) -> bool:
        return shutil.which(command) is not None

    def _wifi_status(self) -> dict[str, object]:
        hostname = socket.gethostname()
        ip_address = ""
        try:
            ip_result = self._run_probe(["ip", "-4", "-o", "addr", "show", "wlan0"], 2)
            match = re.search(r"\binet\s+([0-9.]+)/", ip_result.stdout)
            ip_address = match.group(1) if match else ""
        except (OSError, subprocess.TimeoutExpired):
            pass

        status: dict[str, object] = {
            "supported": self._command_available("iw") or self._command_available("nmcli"),
            "interface": "wlan0",
            "hostname": hostname,
            "ip_address": ip_address,
            "ssid": "",
            "signal_percent": None,
            "saved_networks": [],
            "internet": None,
        }
        try:
            link = self._run_probe(["iw", "dev", "wlan0", "link"], 3)
            ssid = re.search(r"SSID:\s*(.+)", link.stdout)
            signal = re.search(r"signal:\s*(-?\d+)", link.stdout)
            if ssid:
                status["ssid"] = ssid.group(1).strip()
            if signal:
                dbm = int(signal.group(1))
                status["signal_percent"] = max(0, min(100, 2 * (dbm + 100)))
        except (OSError, subprocess.TimeoutExpired, ValueError):
            pass
        if self._command_available("nmcli"):
            try:
                saved = self._run_probe(["nmcli", "-t", "-f", "NAME,TYPE", "connection", "show"], 4)
                status["saved_networks"] = [
                    line.split(":", 1)[0]
                    for line in saved.stdout.splitlines()
                    if line.endswith(":802-11-wireless") and line.split(":", 1)[0]
                ]
            except (OSError, subprocess.TimeoutExpired):
                pass
        return status

    def _wifi_scan(self) -> dict[str, object]:
        if not self._command_available("nmcli"):
            return {"supported": False, "networks": [], "reason": "NetworkManager nmcli is unavailable on SHAeR."}
        try:
            result = self._run_probe(["nmcli", "-t", "-f", "SSID,SIGNAL,SECURITY,IN-USE", "dev", "wifi", "list", "ifname", "wlan0", "--rescan", "yes"], 8)
        except (OSError, subprocess.TimeoutExpired):
            return {"supported": False, "networks": [], "reason": "SHAeR could not complete a Wi-Fi scan."}
        networks = []
        seen: set[str] = set()
        for line in result.stdout.splitlines():
            parts = line.split(":")
            if len(parts) < 4:
                continue
            ssid = parts[0].strip()
            if not ssid or ssid in seen:
                continue
            seen.add(ssid)
            networks.append({
                "ssid": ssid,
                "signal_percent": int(parts[1]) if parts[1].isdigit() else None,
                "security": parts[2] or "open",
                "current": parts[3].strip() == "*",
            })
        return {"supported": result.returncode == 0, "networks": networks, "stderr": result.stderr.strip()[:240] if result.returncode else ""}

    def _bluetooth_status(self) -> dict[str, object]:
        if not self._command_available("bluetoothctl"):
            return {"supported": False, "enabled": False, "discoverable": False, "paired_devices": [], "active_device": None, "reason": "bluetoothctl is unavailable on SHAeR."}
        try:
            show = self._run_probe(["bluetoothctl", "show"], 4)
            paired = self._run_probe(["bluetoothctl", "paired-devices"], 4)
        except (OSError, subprocess.TimeoutExpired):
            return {"supported": False, "enabled": False, "discoverable": False, "paired_devices": [], "active_device": None, "reason": "SHAeR could not read Bluetooth status."}
        devices = []
        for line in paired.stdout.splitlines():
            match = re.match(r"Device\s+([0-9A-Fa-f:]{17})\s+(.+)", line)
            if match:
                devices.append({"id": match.group(1), "name": match.group(2), "state": "paired"})
        return {
            "supported": show.returncode == 0,
            "enabled": "Powered: yes" in show.stdout,
            "discoverable": "Discoverable: yes" in show.stdout,
            "paired_devices": devices,
            "active_device": None,
            "stderr": show.stderr.strip()[:240] if show.returncode else "",
        }

    def _bluetooth_scan(self) -> dict[str, object]:
        if not self._command_available("bluetoothctl"):
            return {"supported": False, "devices": [], "reason": "bluetoothctl is unavailable on SHAeR."}
        try:
            result = self._run_probe(["bluetoothctl", "devices"], 4)
        except (OSError, subprocess.TimeoutExpired):
            return {"supported": False, "devices": [], "reason": "SHAeR could not read Bluetooth devices."}
        devices = []
        for line in result.stdout.splitlines():
            match = re.match(r"Device\s+([0-9A-Fa-f:]{17})\s+(.+)", line)
            if match:
                devices.append({"id": match.group(1), "name": match.group(2), "kind": "unknown", "state": "discovered"})
        return {"supported": result.returncode == 0, "devices": devices, "stderr": result.stderr.strip()[:240] if result.returncode else ""}

    def _companion_authenticate(self) -> dict[str, object]:
        authorization = self.headers.get("Authorization", "")
        token = authorization[7:].strip() if authorization.lower().startswith("bearer ") else None
        return self.companion.pairing.authenticate(token)

    @staticmethod
    def _static_path_allowed(request_path: str) -> bool:
        decoded = unquote(request_path)
        parts = [part for part in Path(decoded).parts if part not in {"/", ""}]
        if not parts or any(part in {".", ".."} or part.startswith(".") for part in parts):
            return False
        theme_roots = {
            "shaer_base_dark", "shaer_base_light",
            "shaer_dark_archive", "shaer_bombay_ticket", "shaer_japanese_punk",
            "shaer_windows_xp", "shaer_ghibli_garden", "shaer_indian_print", "shaer_pixel_ui",
        }
        if parts[0] in theme_roots:
            return len(parts) == 1 or parts[1] == "index.html" or parts[1] in {"src", "assets", "themes"}
        if parts[0] == "shaer_companion":
            return len(parts) == 1 or parts[1] in {"index.html", "manifest.webmanifest", "service-worker.js", "src", "icons"}
        if parts[0] == "shaer_pi_os":
            return len(parts) == 2 and parts[1] in {"music-store.js", "firmware-core.js", "hardware-bridge.js", "system-overlays.css"}
        return False

    def _require_local_device(self) -> None:
        if self.client_address[0] not in {"127.0.0.1", "::1"}:
            raise ApiError("device_approval_required", "Pairing must be approved on SHAeR itself.", 403)

    @staticmethod
    def _require_physical_capability(payload: dict[str, object], pairing_id: str | None = None) -> None:
        if not consume_physical_capability(payload.get("physical_nonce"), pairing_id):
            raise ApiError("physical_confirmation_required", "Press SHAeR's OK button to confirm this action.", 403)

    def _spotify_status(self) -> dict[str, object]:
        try:
            return self.spotify.status()
        except Exception:
            return {"authenticated": False}

    def _playback_status(self) -> dict[str, object]:
        try:
            local = self._local_playback_status()
            if str(local.get("status") or "").lower() in {"playing", "paused", "buffering"} and local.get("uri"):
                return local
            return self.spotify.playback() if self.spotify.status().get("authenticated") else local
        except Exception:
            return {}

    @staticmethod
    def _public_local_playback(track: dict[str, object], status: str = "playing") -> dict[str, object]:
        track_id = str(track.get("id") or track.get("media_id") or "")
        filepath = str(track.get("filepath") or "")
        return {
            "title": str(track.get("title") or "Unknown track"),
            "artist": str(track.get("artist") or "Unknown artist"),
            "album": str(track.get("album") or "Unknown album"),
            "duration_ms": max(0, int(track.get("duration_s") or 0) * 1000),
            "progress_ms": 0,
            "cover_art": str(track.get("cover_art_path")) if track.get("cover_art_path") else None,
            "status": status,
            "queue_position": None,
            "volume_percent": None,
            "source": "local",
            "uri": f"local:track:{track_id}" if track_id else filepath,
            "id": track_id,
            "filepath": filepath,
        }

    def _local_track(self, payload: dict[str, object]) -> dict[str, object]:
        requested = str(payload.get("id") or payload.get("media_id") or "").strip()
        requested_uri = str(payload.get("uri") or "").strip()
        if requested_uri.startswith("local:track:"):
            requested = requested_uri.rsplit(":", 1)[-1]
        result = self.companion.music("")
        data = result.get("data", {}) if isinstance(result, dict) else {}
        tracks = data.get("tracks", []) if isinstance(data, dict) else []
        for item in tracks if isinstance(tracks, list) else []:
            if not isinstance(item, dict):
                continue
            item_id = str(item.get("id") or item.get("media_id") or "")
            if requested and item_id == requested:
                if not item.get("filepath"):
                    raise ApiError("local_track_unavailable", "This local track has no playable file path.", 409)
                return item
        raise ApiError("local_track_not_found", "Local track does not exist.", 404)

    def _local_playback_status(self) -> dict[str, object]:
        with LOCAL_PLAYBACK_LOCK:
            return dict(LOCAL_PLAYBACK_STATE)

    def _local_playback_post(self, payload: dict[str, object]) -> dict[str, object]:
        action = str(payload.get("action") or "play-track")
        with LOCAL_PLAYBACK_LOCK:
            current = dict(LOCAL_PLAYBACK_STATE)
        if action in {"play-track", "play-uri"}:
            track = self._local_track(payload)
            next_state = self._public_local_playback(track, "playing")
        elif action == "toggle-play":
            next_state = {**current, "status": "paused" if current.get("status") == "playing" else "playing"}
        elif action in {"play", "pause", "stop"}:
            next_state = {**current, "status": "playing" if action == "play" else "paused" if action == "pause" else "stopped"}
        else:
            raise ApiError("invalid_local_playback_action", "Unknown local playback action.")
        if action != "play-track" and not next_state.get("uri"):
            raise ApiError("local_playback_empty", "No local track is selected.", 409)
        with LOCAL_PLAYBACK_LOCK:
            LOCAL_PLAYBACK_STATE.clear()
            LOCAL_PLAYBACK_STATE.update(next_state)
            return dict(LOCAL_PLAYBACK_STATE)

    def _read_json(self, maximum: int) -> dict[str, object]:
        length = max(0, int(self.headers.get("Content-Length", "0") or 0))
        if length > maximum:
            raise ApiError("request_too_large", "Request exceeds the endpoint limit.", 413)
        payload = json.loads(self.rfile.read(length).decode("utf-8")) if length else {}
        if not isinstance(payload, dict):
            raise ApiError("invalid_json", "JSON body must be an object.")
        return payload

    def _send_companion_error(self, exc: Exception) -> None:
        if isinstance(exc, ApiError):
            self.send_json(exc.payload(), status=exc.status)
        elif isinstance(exc, ArchiveError):
            self.send_json({"version": PROTOCOL_VERSION, "ok": False, "error": {"code": exc.code, "message": str(exc)}}, status=exc.status)
        elif isinstance(exc, RecordingError):
            status = 404 if exc.code == "recording_not_found" else 400
            self.send_json(ApiError(exc.code, str(exc), status).payload(), status=status)
        elif isinstance(exc, SpotifyApiError):
            status = self._spotify_error_status(exc)
            detail = {"code": "spotify_error", "message": str(exc)}
            if exc.reason:
                detail["reason"] = exc.reason
            self.send_json({"version": PROTOCOL_VERSION, "ok": False, "error": detail}, status=status)
        elif isinstance(exc, SpotifyAuthError):
            self.send_json(ApiError("spotify_authentication_required", str(exc), 401).payload(), status=401)
        else:
            self.send_json(ApiError("invalid_request", str(exc)).payload(), status=400)

    def _system_post(self, path: str, payload: dict[str, object]) -> dict[str, object]:
        if path == "/api/system/metrics":
            numeric = {
                "fps": (int, float), "frames": (int, float), "navigationLatencyMs": (int, float),
                "spotifyLatencyMs": (int, float), "artworkLatencyMs": (int, float), "errors": (int, float),
                "uptimeMs": (int, float), "usedJsHeapBytes": (int, float), "totalJsHeapBytes": (int, float),
            }
            sample = {
                key: round(float(value), 3)
                for key, value in payload.items()
                if key in numeric and isinstance(value, numeric[key]) and not isinstance(value, bool)
            }
            theme = payload.get("theme")
            if isinstance(theme, str) and theme in {"base-dark", "base-light", "archive", "bombay", "punk", "xp", "garden", "raga"}:
                sample["theme"] = theme
            if len(sample) != len(payload):
                raise ApiError("invalid_metrics", "Metrics payload contains unsupported fields.")
            sample["received_at"] = time.time()
            with METRICS_LOCK:
                METRICS.append(sample)
                del METRICS[:-120]
            return {"ok": True}
        if path == "/api/system/shutdown":
            if not self.allow_power:
                raise ApiError("power_disabled", "Hardware shutdown is disabled in this runtime.", 403)
            self._require_physical_capability(payload)
            subprocess.Popen(
                ["sudo", "-n", "/usr/bin/systemctl", "poweroff"],
                stdin=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                start_new_session=True,
            )
            return {"ok": True, "shutting_down": True}
        raise ValueError("Unknown system endpoint.")

    def _spotify_get(self, path: str, query: dict[str, list[str]]) -> None:
        try:
            self._require_local_device()
            if path == "/api/spotify/status":
                result = self.spotify.status()
            elif path == "/api/spotify/login":
                launch = (query.get("launch") or ["1"])[0] not in {"0", "false", "no"}
                timeout = min(900, max(60, int((query.get("timeout") or ["300"])[0])))
                result = self.spotify.begin_login(launch_browser=launch, timeout_s=timeout)
            elif path == "/api/spotify/callback":
                result = self.spotify.callback(query)
                self.send_html(
                    "<meta http-equiv='refresh' content='2;url=/'>"
                    "<h1>SHAeR is connected to Spotify</h1>"
                    "<p>Returning to SHAeR...</p>"
                )
                return
            elif path == "/api/spotify/me":
                result = self.spotify.profile()
            elif path.startswith("/api/spotify/library/"):
                result = self.spotify.library(path.rsplit("/", 1)[-1], query)
            elif path.startswith("/api/spotify/playlists/") and path.endswith("/tracks"):
                playlist_id = path.split("/")[4]
                result = self.spotify.playlist_items(playlist_id, query)
            elif path == "/api/spotify/search":
                result = self.spotify.search(query)
            elif path == "/api/spotify/playback":
                result = self.spotify.playback()
            elif path == "/api/spotify/queue":
                result = self.spotify.queue()
            elif path == "/api/spotify/connect/status":
                result = self.spotify.connect_status()
            else:
                self.send_json({"ok": False, "error": "Unknown Spotify endpoint."}, status=404)
                return
            self.send_json(result if isinstance(result, dict) else {"data": result})
        except ApiError as exc:
            self._send_companion_error(exc)
        except (SpotifyAuthError, SpotifyApiError, ValueError, OSError, RuntimeError) as exc:
            if path == "/api/spotify/callback":
                self.send_html(f"<h1>Spotify login failed</h1><p>{self._html_escape(str(exc))}</p>", status=400)
                return
            self.send_json({"ok": False, "error": str(exc)}, status=self._spotify_error_status(exc))

    def _spotify_post(self, path: str, payload: dict[str, object]) -> dict[str, object]:
        if path == "/api/spotify/logout":
            return self.spotify.logout()
        if path == "/api/spotify/cancel":
            state = str(payload.get("state") or "") or None
            return self.spotify.cancel(state)
        if path == "/api/spotify/connect/transfer":
            return self.spotify.connect_transfer(bool(payload.get("play", False)))
        if path == "/api/spotify/control":
            value = payload.get("value")
            return self.spotify.control(
                str(payload.get("action") or ""),
                int(value) if value is not None else None,
                str(payload.get("uri") or "") or None,
                str(payload.get("context_uri") or "") or None,
                bool(payload.get("enabled")) if "enabled" in payload else None,
                str(payload.get("mode") or "") or None,
            )
        raise ValueError("Unknown Spotify endpoint.")

    @staticmethod
    def _spotify_error_status(exc: Exception) -> int:
        if isinstance(exc, SpotifyApiError) and exc.status:
            return exc.status
        if isinstance(exc, SpotifyAuthError):
            return 401
        return 400

    @staticmethod
    def _html_escape(value: str) -> str:
        return value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

    def send_html(self, content: str, status: int = 200) -> None:
        body = ("<!doctype html><meta charset='utf-8'><title>SHAeR Spotify</title>" + content).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_json(self, payload: dict[str, object], status: int = 200) -> None:
        body = json.dumps(payload).encode("utf-8")
        try:
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            self.close_connection = True


def start_gpio(args: argparse.Namespace) -> None:
    try:
        controller = GpioInputController(
            lambda event: push_event(str(event.action), event.source),
            pin_a=args.pin_a,
            pin_b=args.pin_b,
            pin_select=args.pin_select,
            pin_back=args.pin_back,
            pin_home=args.pin_home,
            bounce_time=args.bounce,
            hold_time=args.hold_time,
        )
        controller.start()
    except Exception as exc:  # pragma: no cover - only exercised on non-Pi hosts.
        print(f"GPIO disabled: input HAL could not start ({exc}).")
        return
    GPIO_DEVICES.append(controller)

    print(
        "GPIO ready: "
        f"encoder=({args.pin_a},{args.pin_b}) select={args.pin_select} "
        f"back={args.pin_back} home={args.pin_home if args.pin_home >= 0 else 'disabled'}"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the SHAeR Pi OS UI.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8775)
    parser.add_argument("--theme", default="auto", help="Theme id, or auto to use the saved display setting.")
    parser.add_argument("--gpio", action="store_true", help="Enable Raspberry Pi GPIO input.")
    parser.add_argument("--pin-a", type=int, default=17, help="Rotary encoder A/CLK GPIO pin.")
    parser.add_argument("--pin-b", type=int, default=27, help="Rotary encoder B/DT GPIO pin.")
    parser.add_argument("--pin-ok", "--pin-select", dest="pin_select", type=int, default=22, help="Separate OK/select push button GPIO pin.")
    parser.add_argument("--pin-back", type=int, default=23, help="Separate Back push button GPIO pin.")
    parser.add_argument("--pin-home", type=int, default=-1, help="Optional home button GPIO pin, or -1.")
    parser.add_argument("--bounce", type=float, default=0.045)
    parser.add_argument("--hold-time", type=float, default=0.8, help="Seconds before OK/Back becomes a long press.")
    parser.add_argument("--allow-power", action="store_true", help="Allow the confirmed system shutdown API.")
    parser.add_argument("--allow-test-input", action="store_true", help="Enable loopback-only POST /api/debug/input for bench testing.")
    return parser.parse_args()


def main() -> None:
    global PENDING_PAIRING_IDS_PROVIDER
    args = parse_args()
    os.chdir(ROOT)

    if args.gpio:
        start_gpio(args)

    spotify = SpotifyRuntime(args.port)
    companion = CompanionService(ROOT)
    PENDING_PAIRING_IDS_PROVIDER = lambda: [
        str(request["pairing_id"]) for request in companion.pairing.pending_for_device()
    ]
    pending_library_restore = companion.library_path.with_suffix(".db.restore-pending")
    if pending_library_restore.exists():
        companion.library_path.parent.mkdir(parents=True, exist_ok=True)
        os.replace(pending_library_restore, companion.library_path)
    pending_recordings_restore = companion.recordings_db.with_suffix(".db.restore-pending")
    if pending_recordings_restore.exists():
        companion.recordings_db.parent.mkdir(parents=True, exist_ok=True)
        os.replace(pending_recordings_restore, companion.recordings_db)
    recording_archive = RecordingArchive(
        Path(os.environ.get("SHAER_RECORDINGS_DB", companion.config_dir / "recordings.db")),
        Path(os.environ.get("SHAER_RECORDINGS_DIR", companion.config_dir / "Recordings")),
    )
    recording_options: dict[str, object] = {
        "max_duration_s": int(os.environ.get("SHAER_RECORDING_MAX_SECONDS", "3600")),
        "minimum_free_bytes": int(os.environ.get("SHAER_RECORDING_MIN_FREE_BYTES", str(128 * 1024 * 1024))),
    }
    if os.environ.get("SHAER_RECORDING_TEST_MODE") == "1":
        recording_options["backend_factory"] = lambda: SyntheticCaptureBackend()
    recording = RecordingService(recording_archive, **recording_options)
    archive = MusicalArchive(
        Path(os.environ.get("SHAER_ARCHIVE_DB", companion.config_dir / "archive.db")),
        Path(os.environ.get("SHAER_MARGINALIA_DIR", companion.config_dir / "marginalia")),
    )
    saved_settings = companion.settings().get("data", {})
    saved_display = saved_settings.get("display", {}) if isinstance(saved_settings, dict) else {}
    theme = str(saved_display.get("theme", "shaer_dark_archive")) if args.theme == "auto" and isinstance(saved_display, dict) else args.theme
    if not (ROOT / theme / "index.html").exists():
        theme = "shaer_dark_archive"

    def handler(*handler_args, **handler_kwargs):
        return ShaerHandler(
            *handler_args,
            theme=theme,
            spotify=spotify,
            companion=companion,
            recording=recording,
            archive=archive,
            allow_power=args.allow_power,
            allow_test_input=args.allow_test_input,
            **handler_kwargs,
        )

    server = ThreadingHTTPServer((args.host, args.port), handler)
    udp_discovery = UdpDiscoveryResponder(companion, spotify, args.port)
    udp_discovery.start()
    url = f"http://{args.host}:{args.port}/{theme}/?mode=device"
    print(f"SHAeR Pi OS serving {ROOT}")
    print(f"Open {url}")
    if args.allow_test_input:
        print("Bench input enabled: POST /api/debug/input with a JSON action.")
    print(f"Spotify configured: {'yes' if spotify.configured else 'no (set SPOTIFY_CLIENT_ID)'}")
    print(f"Companion: http://{args.host}:{args.port}/companion (protocol v1)")
    stopping = threading.Event()

    def stop_server(_signum=None, _frame=None) -> None:
        if stopping.is_set():
            return
        stopping.set()
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, stop_server)
    signal.signal(signal.SIGINT, stop_server)
    try:
        server.serve_forever()
    finally:
        server.server_close()
        udp_discovery.stop()
        recording_archive.close()
        archive.close()
        for device in GPIO_DEVICES:
            close = getattr(device, "close", None)
            if callable(close):
                close()
        print("\nStopped SHAeR Pi OS cleanly.")


if __name__ == "__main__":
    main()
