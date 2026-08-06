"""Versioned, transport-independent SHAeR companion protocol core."""

from __future__ import annotations

import base64
import hashlib
import hmac
import io
import json
import os
import re
import secrets
import shutil
import sqlite3
import stat
import subprocess
import tempfile
import threading
import time
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Mapping


PROTOCOL_VERSION = 1
PAIRING_TTL_S = 180
MAX_JSON_FILE_BYTES = 64 * 1024 * 1024
MAX_BACKUP_BYTES = 256 * 1024 * 1024
MAX_ARCHIVE_EXPANDED_BYTES = 1024 * 1024 * 1024
DEFAULT_CONFIG_DIR = Path(os.environ.get("SHAER_CONFIG_DIR", Path.home() / ".config" / "shaer"))
MAX_ARCHIVE_MEMBERS = 10_000
MAX_ARCHIVE_RATIO = 200
MAX_PAIRING_SESSIONS = 3
PAIRING_RATE_LIMIT_S = 5.0
SUPPORTED_AUDIO_EXTENSIONS = {".mp3", ".flac", ".wav", ".aac", ".ogg", ".m4a"}
THEME_ID = re.compile(r"^shaer_[a-z0-9_]{2,48}$")
BUILT_IN_THEMES = {
    "shaer_base_dark": "Base Dark",
    "shaer_base_light": "Base Light",
    "shaer_dark_archive": "Dark Archive",
    "shaer_bombay_ticket": "Bombay Ticket",
    "shaer_japanese_punk": "Japanese Punk",
    "shaer_windows_xp": "Windows XP",
    "shaer_ghibli_garden": "Ghibli Garden",
    "shaer_indian_print": "Indian Raga",
}

CAPABILITY_STATES = {"supported", "unsupported", "temporarily_unavailable", "permission_required", "hardware_missing"}
SETTINGS_DOMAIN_MAP = {
    "appearance": "display",
    "playback": "spotify",
    "audio": "audio",
    "connectivity": "wifi",
    "power": "power",
    "datetime": "display",
    "sync": "library",
    "advanced": "developer",
}


class ApiError(RuntimeError):
    def __init__(self, code: str, message: str, status: int = 400):
        super().__init__(message)
        self.code = code
        self.status = status

    def payload(self) -> dict[str, object]:
        return {
            "version": PROTOCOL_VERSION,
            "ok": False,
            "error": {"code": self.code, "message": str(self)},
        }


class JsonStore:
    def __init__(self, path: str | Path, default: Mapping[str, object]):
        self.path = Path(path).expanduser()
        self.default = dict(default)
        self.lock = threading.RLock()

    def load(self) -> dict[str, object]:
        with self.lock:
            return self._load_unlocked()

    def save(self, payload: Mapping[str, object]) -> None:
        with self.lock:
            self._save_unlocked(payload)

    def mutate(self, callback: Callable[[dict[str, object]], Any]) -> Any:
        """Apply a read-modify-write operation while holding one store lock."""
        with self.lock:
            payload = self._load_unlocked()
            result = callback(payload)
            self._save_unlocked(payload)
            return result

    def _load_unlocked(self) -> dict[str, object]:
        if not self.path.exists():
            return json.loads(json.dumps(self.default))
        try:
            payload = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ApiError("storage_corrupt", f"Cannot read {self.path.name}.", 500) from exc
        if not isinstance(payload, dict):
            raise ApiError("storage_corrupt", f"Invalid {self.path.name} format.", 500)
        return payload

    def _save_unlocked(self, payload: Mapping[str, object]) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        fd, temporary = tempfile.mkstemp(prefix=".shaer-", dir=self.path.parent)
        try:
            os.fchmod(fd, 0o600)
            with os.fdopen(fd, "w", encoding="utf-8") as handle:
                json.dump(payload, handle, indent=2, sort_keys=True)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, self.path)
            self.path.chmod(0o600)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


DEFAULT_SETTINGS: dict[str, object] = {
    "audio": {
        "volume_percent": 50, "volume_limit": 90, "equalizer_preset": "flat", "bass": 0, "mid": 0,
        "treble": 0, "loudness": False, "replay_gain": True, "crossfade_s": 0,
        "channels": "stereo",
    },
    "display": {"brightness": 80, "sleep_timeout_s": 120, "animation_speed": 1.0, "theme": "shaer_dark_archive", "clock_format": "12h"},
    "bluetooth": {"device_name": "SHAeR", "pairing": True, "auto_connect": True, "codec": "auto"},
    "wifi": {"dhcp": True, "hostname": "shaer", "saved_networks": []},
    "power": {"auto_shutdown_min": 0, "low_battery_percent": 15, "charging_behavior": "normal"},
    "library": {"scan_paths": [str(Path(os.environ.get("SHAER_MUSIC_DIR", DEFAULT_CONFIG_DIR / "music")))], "automatic_rescan": True, "artwork_cache_mb": 256},
    "spotify": {"device_name": "SHAeR"},
    "privacy": {"listening_history": True, "analytics": False},
    "developer": {"ssh": False, "logging_level": "info", "debug_overlays": False},
}


@dataclass(slots=True)
class PairingSession:
    pairing_id: str
    device_name: str
    code: str
    expires_at: float
    source: str
    approved: bool = False
    denied: bool = False
    delivered: bool = False


class PairingManager:
    def __init__(self, trusted_store: JsonStore):
        self.store = trusted_store
        self.sessions: dict[str, PairingSession] = {}
        self.last_start_by_source: dict[str, float] = {}
        self.lock = threading.RLock()

    def start(self, device_name: str, source: str = "unknown") -> dict[str, object]:
        clean_name = device_name.strip()[:48]
        if not clean_name:
            raise ApiError("invalid_device_name", "A companion device name is required.")
        with self.lock:
            self._expire()
            now = time.time()
            clean_source = source.strip()[:64] or "unknown"
            if len(self.sessions) >= MAX_PAIRING_SESSIONS:
                raise ApiError("pairing_busy", "SHAeR already has too many pending pairing requests.", 429)
            if now - self.last_start_by_source.get(clean_source, 0) < PAIRING_RATE_LIMIT_S:
                raise ApiError("pairing_rate_limited", "Wait before starting another pairing request.", 429)
            session = PairingSession(
                pairing_id=secrets.token_urlsafe(32),
                device_name=clean_name,
                code=f"{secrets.randbelow(1_000_000):06d}",
                expires_at=now + PAIRING_TTL_S,
                source=clean_source,
            )
            self.sessions[session.pairing_id] = session
            self.last_start_by_source[clean_source] = now
        return {
            "pairing_id": session.pairing_id,
            "code": session.code,
            "expires_at": int(session.expires_at),
            "requires_device_approval": True,
        }

    def pending_for_device(self) -> list[dict[str, object]]:
        with self.lock:
            self._expire()
            return [
                {"pairing_id": session.pairing_id, "device_name": session.device_name, "code": session.code, "expires_at": int(session.expires_at)}
                for session in self.sessions.values() if not session.approved and not session.denied
            ]

    def approve(self, pairing_id: str, approved: bool) -> None:
        with self.lock:
            session = self._session(pairing_id)
            session.approved = approved
            session.denied = not approved

    def status(self, pairing_id: str) -> dict[str, object]:
        with self.lock:
            session = self._session(pairing_id)
            if session.denied:
                return {"state": "denied"}
            if not session.approved:
                return {"state": "pending", "expires_at": int(session.expires_at)}
            if session.delivered:
                return {"state": "paired"}
            token = secrets.token_urlsafe(48)
            device_id = secrets.token_urlsafe(12)
            def add_device(trusted: dict[str, object]) -> None:
                devices = trusted.setdefault("devices", [])
                if not isinstance(devices, list):
                    devices = []
                    trusted["devices"] = devices
                devices.append({
                    "id": device_id,
                    "name": session.device_name,
                    "role": "owner",
                    "permissions": ["*"],
                    "token_hash": hashlib.sha256(token.encode("utf-8")).hexdigest(),
                    "paired_at": int(time.time()),
                    "last_seen_at": int(time.time()),
                })
            self.store.mutate(add_device)
            session.delivered = True
            return {
                "state": "paired",
                "token": token,
                "device": {"id": device_id, "name": session.device_name, "role": "owner"},
            }

    def authenticate(self, token: str | None) -> dict[str, object]:
        if not token:
            raise ApiError("authentication_required", "Pair this companion with SHAeR first.", 401)
        digest = hashlib.sha256(token.encode("utf-8")).hexdigest()
        def touch_device(trusted: dict[str, object]) -> dict[str, object] | None:
            devices = trusted.get("devices", [])
            if not isinstance(devices, list):
                return None
            for device in devices:
                if isinstance(device, dict) and secrets.compare_digest(str(device.get("token_hash") or ""), digest):
                    device["last_seen_at"] = int(time.time())
                    return device
            return None
        device = self.store.mutate(touch_device)
        if device is not None:
            return device
        raise ApiError("invalid_token", "This companion is not trusted by SHAeR.", 401)

    def trusted_devices(self) -> list[dict[str, object]]:
        devices = self.store.load().get("devices", [])
        if not isinstance(devices, list):
            return []
        return [
            {key: value for key, value in device.items() if key != "token_hash"}
            for device in devices if isinstance(device, dict)
        ]

    def forget(self, device_id: str) -> bool:
        def remove_device(trusted: dict[str, object]) -> bool:
            devices = trusted.get("devices", [])
            if not isinstance(devices, list):
                return False
            kept = [device for device in devices if not isinstance(device, dict) or device.get("id") != device_id]
            trusted["devices"] = kept
            return len(kept) != len(devices)
        return bool(self.store.mutate(remove_device))

    def _session(self, pairing_id: str) -> PairingSession:
        self._expire()
        session = self.sessions.get(pairing_id)
        if not session:
            raise ApiError("pairing_not_found", "Pairing request expired or does not exist.", 404)
        return session

    def _expire(self) -> None:
        now = time.time()
        for pairing_id in [key for key, session in self.sessions.items() if session.expires_at <= now]:
            self.sessions.pop(pairing_id, None)


class CompanionService:
    def __init__(self, root: str | Path, config_dir: str | Path | None = None):
        self.root = Path(root)
        self.config_dir = Path(config_dir or os.environ.get("SHAER_CONFIG_DIR", "~/.config/shaer")).expanduser()
        self.settings_store = JsonStore(self.config_dir / "settings.json", DEFAULT_SETTINGS)
        self.pairing = PairingManager(JsonStore(self.config_dir / "trusted-companions.json", {"devices": []}))
        self.music_root = Path(os.environ.get("SHAER_MUSIC_DIR", self.config_dir / "music")).expanduser()
        self.library_path = Path(os.environ.get("SHAER_LIBRARY_DB", self.config_dir / "library.db")).expanduser()
        self.backup_dir = Path(os.environ.get("SHAER_BACKUP_DIR", self.config_dir / "backups")).expanduser()
        self.update_dir = Path(os.environ.get("SHAER_UPDATE_DIR", self.config_dir / "updates")).expanduser()
        self.recordings_root = Path(os.environ.get("SHAER_RECORDINGS_DIR", self.config_dir / "Recordings")).expanduser()
        self.recordings_db = Path(os.environ.get("SHAER_RECORDINGS_DB", self.config_dir / "recordings.db")).expanduser()
        self.music_root.mkdir(parents=True, exist_ok=True)
        self.started_at = time.time()

    @staticmethod
    def envelope(data: object) -> dict[str, object]:
        return {"version": PROTOCOL_VERSION, "ok": True, "data": data}

    def discovery(self, spotify_status: Mapping[str, object] | None = None) -> dict[str, object]:
        settings = self.settings_store.load()
        display = settings.get("display", {}) if isinstance(settings.get("display"), dict) else {}
        usage = shutil.disk_usage(self.root)
        return self.envelope({
            "device_id": self.device_id(),
            "device_name": "SHAeR",
            "firmware_version": "0.16.0",
            "protocol_version": PROTOCOL_VERSION,
            "battery_percent": None,
            "current_theme": display.get("theme", "shaer_dark_archive"),
            "storage": {"total": usage.total, "used": usage.used, "free": usage.free},
            "spotify_status": "connected" if spotify_status and spotify_status.get("authenticated") else "not_connected",
            "bluetooth_status": None,
            "wifi_status": "connected",
            "signal_strength_dbm": self._wifi_signal(),
            "pairing_available": True,
        })

    def device_id(self) -> str:
        """Return one stable, installation-local identity for discovery deduplication."""
        path = self.config_dir / "device_id"
        try:
            value = path.read_text(encoding="utf-8").strip()
            if value:
                return value
            value = f"shaer-{secrets.token_hex(8)}"
            path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
            path.write_text(value + "\n", encoding="utf-8")
            os.chmod(path, 0o600)
            return value
        except OSError:
            return "shaer-local"

    def dashboard(self, spotify_status: Mapping[str, object] | None = None, playback: Mapping[str, object] | None = None) -> dict[str, object]:
        usage = shutil.disk_usage(self.root)
        settings = self.settings_store.load()
        display = settings.get("display", {}) if isinstance(settings.get("display"), dict) else {}
        return self.envelope({
            "now_playing": dict(playback or {}),
            "battery_percent": None,
            "charging": None,
            "storage": {"total": usage.total, "used": usage.used, "free": usage.free},
            "current_theme": display.get("theme", "shaer_dark_archive"),
            "firmware_version": "0.16.0",
            "cpu_temperature_c": self._temperature(),
            "uptime_s": int(time.time() - self.started_at),
            "connection_quality": "wifi",
            "spotify_authenticated": bool(spotify_status and spotify_status.get("authenticated")),
        })

    def device(self, spotify_status: Mapping[str, object] | None = None) -> dict[str, object]:
        settings = self.settings_store.load()
        display = settings.get("display", {}) if isinstance(settings.get("display"), dict) else {}
        return self.envelope({
            "device_id": self.device_id(),
            "device_name": "SHAeR",
            "os_name": "adi-vasi OS",
            "os_version": "0.16.0",
            "protocol_version": PROTOCOL_VERSION,
            "active_theme": display.get("theme", "shaer_dark_archive"),
            "spotify": {
                "capability": self._capability("supported"),
                "authenticated": bool(spotify_status and spotify_status.get("authenticated")),
            },
        })

    def capabilities(self, *, power_actions: bool = False, hardware_input: bool = False, recording: bool = True) -> dict[str, object]:
        return self.envelope({
            "capabilities": {
                "appearance": self._capability("supported"),
                "playback": self._capability("supported"),
                "audio": self._capability("supported"),
                "connectivity": {
                    "state": "supported",
                    "wifi": self._capability("supported"),
                    "bluetooth": self._capability("hardware_missing", "Bluetooth adapter status is not measurable in this host runtime."),
                    "companion_actions": self._capability("supported"),
                },
                "power": {
                    "state": "supported" if power_actions else "permission_required",
                    "actions": self._capability("supported" if power_actions else "permission_required", "Start with --allow-power to enable shutdown/restart actions."),
                    "battery": self._capability("hardware_missing", "No fuel gauge telemetry is exposed in this runtime."),
                    "charging_current": self._capability("hardware_missing", "Charging current is not measurable in this runtime."),
                },
                "datetime": self._capability("permission_required", "Manual system time writes require an authorized OS adapter."),
                "sync": self._capability("supported"),
                "advanced": self._capability("supported"),
                "recording": self._capability("supported" if recording else "hardware_missing"),
                "hardware_input": self._capability("supported" if hardware_input else "hardware_missing"),
            }
        })

    def storage(self) -> dict[str, object]:
        usage = shutil.disk_usage(self.root)
        return self.envelope({
            "sd_card": {
                "capability": self._capability("temporarily_unavailable", "Host runtime cannot distinguish SD card mount state."),
                "state": "unknown",
            },
            "storage": {"total": usage.total, "used": usage.used, "free": usage.free},
        })

    def power(self) -> dict[str, object]:
        return self.envelope({
            "external_power": self._capability("hardware_missing", "External power presence is not measurable in this runtime."),
            "charging_current": self._capability("hardware_missing", "Charging current is not measurable in this runtime."),
            "battery_percent": None,
            "battery_health": self._capability("hardware_missing"),
            "charge_cycles": self._capability("hardware_missing"),
        })

    def settings(self) -> dict[str, object]:
        return self.envelope(self.settings_store.load())

    def settings_domain(self, domain: str) -> dict[str, object]:
        normalized = domain.strip().lower().replace("-", "_")
        if normalized not in SETTINGS_DOMAIN_MAP:
            raise ApiError("not_found", "Unknown settings domain.", 404)
        settings = self.settings_store.load()
        category = SETTINGS_DOMAIN_MAP[normalized]
        values = settings.get(category, {}) if isinstance(settings.get(category), dict) else {}
        return self.envelope({
            "domain": normalized,
            "source_category": category,
            "values": values,
            "fields": [
                {
                    "key": key,
                    "value": value,
                    "source": "settings_store",
                    "capability": self._capability("supported"),
                    "writable": self._capability("supported"),
                    "restart_required": False,
                }
                for key, value in values.items()
            ],
        })

    def update_settings(self, patch: Mapping[str, object]) -> dict[str, object]:
        def apply_patch(settings: dict[str, object]) -> dict[str, object]:
            for category, values in patch.items():
                if category not in DEFAULT_SETTINGS or not isinstance(values, dict):
                    raise ApiError("invalid_setting", f"Unknown settings category: {category}")
                allowed = DEFAULT_SETTINGS[category]
                if not isinstance(allowed, dict):
                    continue
                target = settings.setdefault(category, {})
                if not isinstance(target, dict):
                    target = {}
                    settings[category] = target
                for key, value in values.items():
                    if key not in allowed:
                        raise ApiError("invalid_setting", f"Unknown setting: {category}.{key}")
                    self._validate_setting_value(category, key, value, allowed[key])
                    target[key] = value
            return settings
        settings = self.settings_store.mutate(apply_patch)
        return self.envelope(settings)

    def themes(self) -> dict[str, object]:
        installed = []
        for path in sorted(self.root.glob("shaer_*")):
            if path.is_dir() and (path / "index.html").exists() and path.name not in {"shaer_pixel_ui", "shaer_companion"}:
                installed.append({"id": path.name, "name": BUILT_IN_THEMES.get(path.name, path.name.removeprefix("shaer_").replace("_", " ").title())})
        settings = self.settings_store.load()
        active = settings.get("display", {}).get("theme") if isinstance(settings.get("display"), dict) else None
        return self.envelope({"active": active, "installed": installed})

    def set_theme(self, theme_id: str) -> dict[str, object]:
        if not (self.root / theme_id / "index.html").exists():
            raise ApiError("theme_not_found", "The requested theme is not installed.", 404)
        return self.update_settings({"display": {"theme": theme_id}})

    def diagnostics(self) -> dict[str, object]:
        diagnostics_dir = self.root / "shaer_backend" / "diagnostics"
        names = sorted(path.stem for path in diagnostics_dir.glob("*_test.py"))
        return self.envelope({"available": names})

    def run_diagnostic(self, name: str, hardware: bool = False) -> dict[str, object]:
        if not name.replace("_", "").isalnum():
            raise ApiError("invalid_diagnostic", "Invalid diagnostic name.")
        script = self.root / "shaer_backend" / "diagnostics" / f"{name}.py"
        if not script.exists():
            raise ApiError("diagnostic_not_found", "Diagnostic does not exist.", 404)
        env = os.environ.copy()
        env["PYTHONPATH"] = str(self.root / "shaer_backend")
        if hardware:
            env["SHAER_HARDWARE"] = "1"
        result = subprocess.run(
            [os.environ.get("PYTHON", "python3"), str(script)],
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
            env=env,
        )
        return self.envelope({"name": name, "passed": result.returncode == 0, "output": result.stdout[-8000:], "error": result.stderr[-4000:]})

    def music(self, query: str = "") -> dict[str, object]:
        with self._database() as connection:
            pattern = f"%{query.strip().lower()}%"
            rows = connection.execute(
                """
                SELECT id, filepath, title, artist, album, duration_s, codec,
                       cover_art_path, is_favourite, source, availability
                  FROM tracks
                 WHERE ? = '%%'
                    OR lower(coalesce(title, '')) LIKE ?
                    OR lower(coalesce(artist, '')) LIKE ?
                    OR lower(coalesce(album, '')) LIKE ?
                 ORDER BY artist, album, track_number, title
                """,
                (pattern, pattern, pattern, pattern),
            )
            tracks = [dict(row) for row in rows]
        return self.envelope({"tracks": tracks, "supported_extensions": sorted(SUPPORTED_AUDIO_EXTENSIONS)})

    def upload_track(self, filename: str, content_base64: str) -> dict[str, object]:
        name = Path(filename).name
        suffix = Path(name).suffix.lower()
        if suffix not in SUPPORTED_AUDIO_EXTENSIONS:
            raise ApiError("unsupported_audio", f"{suffix or 'File'} is not a supported audio format.")
        try:
            payload = base64.b64decode(content_base64, validate=True)
        except (ValueError, TypeError) as exc:
            raise ApiError("invalid_file", "Audio payload is not valid base64.") from exc
        if not payload or len(payload) > MAX_JSON_FILE_BYTES:
            raise ApiError("file_size", "Audio file must be between 1 byte and 64 MB.", 413)
        if not self._audio_signature_matches(suffix, payload):
            raise ApiError("invalid_audio", "The file contents do not match the selected audio format.", 422)
        target = self._unique_path(self.music_root, name)
        self._atomic_bytes(target, payload)
        report = self._index_library()
        return self.envelope({"path": str(target.relative_to(self.music_root)), "size": len(payload), "index": report})

    def update_track(self, track_id: int, fields: Mapping[str, object]) -> dict[str, object]:
        allowed = {"title", "artist", "album", "is_favourite"}
        updates = {key: value for key, value in fields.items() if key in allowed}
        if not updates or len(updates) != len(fields):
            raise ApiError("invalid_track_update", "Only title, artist, album, and is_favourite may be changed.")
        assignments = ", ".join(f"{key} = ?" for key in updates)
        values = [1 if key == "is_favourite" and bool(value) else value for key, value in updates.items()]
        with self._database() as connection:
            cursor = connection.execute(f"UPDATE tracks SET {assignments} WHERE id = ?", (*values, track_id))
            if cursor.rowcount != 1:
                raise ApiError("track_not_found", "Track does not exist.", 404)
            connection.commit()
        return self.envelope({"id": track_id, "updated": updates})

    def delete_track(self, track_id: int) -> dict[str, object]:
        with self._database() as connection:
            row = connection.execute("SELECT filepath, source FROM tracks WHERE id = ?", (track_id,)).fetchone()
            if not row:
                raise ApiError("track_not_found", "Track does not exist.", 404)
            path = Path(str(row["filepath"]))
            connection.execute("DELETE FROM playlist_tracks WHERE track_id = ?", (track_id,))
            connection.execute("DELETE FROM tracks WHERE id = ?", (track_id,))
            connection.commit()
        if row["source"] != "spotify" and self._is_within(path, self.music_root):
            path.unlink(missing_ok=True)
        return self.envelope({"id": track_id, "deleted": True})

    def playlists(self) -> dict[str, object]:
        with self._database() as connection:
            rows = connection.execute(
                """
                SELECT playlists.id, playlists.name, playlists.spotify_playlist_id,
                       count(playlist_tracks.track_id) AS track_count
                  FROM playlists LEFT JOIN playlist_tracks ON playlists.id = playlist_tracks.playlist_id
                 GROUP BY playlists.id ORDER BY playlists.name
                """
            )
            items = [dict(row) for row in rows]
        return self.envelope({"playlists": items})

    def playlist(self, playlist_id: int) -> dict[str, object]:
        with self._database() as connection:
            item = connection.execute("SELECT * FROM playlists WHERE id = ?", (playlist_id,)).fetchone()
            if not item:
                raise ApiError("playlist_not_found", "Playlist does not exist.", 404)
            tracks = connection.execute(
                """SELECT tracks.id, tracks.title, tracks.artist, tracks.album, tracks.duration_s,
                          playlist_tracks.position
                     FROM playlist_tracks JOIN tracks ON tracks.id = playlist_tracks.track_id
                    WHERE playlist_tracks.playlist_id = ? ORDER BY playlist_tracks.position""",
                (playlist_id,),
            )
            return self.envelope({"playlist": dict(item), "tracks": [dict(row) for row in tracks]})

    def create_playlist(self, name: str, track_ids: list[int] | None = None) -> dict[str, object]:
        clean_name = name.strip()[:80]
        if not clean_name:
            raise ApiError("invalid_playlist", "Playlist name is required.")
        with self._database() as connection:
            valid_ids = self._validated_track_ids(connection, track_ids or [])
            cursor = connection.execute("INSERT INTO playlists(name, created_at) VALUES (?, ?)", (clean_name, int(time.time())))
            playlist_id = int(cursor.lastrowid)
            for position, track_id in enumerate(valid_ids, 1):
                connection.execute(
                    "INSERT OR REPLACE INTO playlist_tracks(playlist_id, track_id, position) VALUES (?, ?, ?)",
                    (playlist_id, int(track_id), position),
                )
            connection.commit()
        return self.playlist(playlist_id)

    def update_playlist(self, playlist_id: int, name: str | None, track_ids: list[int] | None) -> dict[str, object]:
        with self._database() as connection:
            if not connection.execute("SELECT 1 FROM playlists WHERE id = ?", (playlist_id,)).fetchone():
                raise ApiError("playlist_not_found", "Playlist does not exist.", 404)
            if name is not None:
                clean_name = name.strip()[:80]
                if not clean_name:
                    raise ApiError("invalid_playlist", "Playlist name is required.")
                connection.execute("UPDATE playlists SET name = ? WHERE id = ?", (clean_name, playlist_id))
            if track_ids is not None:
                valid_ids = self._validated_track_ids(connection, track_ids)
                connection.execute("DELETE FROM playlist_tracks WHERE playlist_id = ?", (playlist_id,))
                for position, track_id in enumerate(valid_ids, 1):
                    connection.execute(
                        "INSERT INTO playlist_tracks(playlist_id, track_id, position) VALUES (?, ?, ?)",
                        (playlist_id, int(track_id), position),
                    )
            connection.commit()
        return self.playlist(playlist_id)

    def delete_playlist(self, playlist_id: int) -> dict[str, object]:
        with self._database() as connection:
            connection.execute("DELETE FROM playlist_tracks WHERE playlist_id = ?", (playlist_id,))
            cursor = connection.execute("DELETE FROM playlists WHERE id = ?", (playlist_id,))
            connection.commit()
        if cursor.rowcount != 1:
            raise ApiError("playlist_not_found", "Playlist does not exist.", 404)
        return self.envelope({"id": playlist_id, "deleted": True})

    def export_playlist(self, playlist_id: int) -> dict[str, object]:
        data = self.playlist(playlist_id)["data"]
        return self.envelope({"format": "shaer-playlist-v1", "content": data})

    def import_playlist(self, payload: Mapping[str, object]) -> dict[str, object]:
        playlist = payload.get("playlist")
        tracks = payload.get("tracks", [])
        if not isinstance(playlist, dict) or not isinstance(tracks, list):
            raise ApiError("invalid_playlist", "Invalid SHAeR playlist document.")
        track_ids = [int(track["id"]) for track in tracks if isinstance(track, dict) and "id" in track]
        return self.create_playlist(str(playlist.get("name") or "Imported Playlist"), track_ids)

    def import_theme(self, filename: str, content_base64: str) -> dict[str, object]:
        if os.environ.get("SHAER_ALLOW_UNSIGNED_THEMES") != "1":
            raise ApiError("theme_import_disabled", "Unsigned theme installation is disabled on this build.", 403)
        try:
            payload = base64.b64decode(content_base64, validate=True)
        except (ValueError, TypeError) as exc:
            raise ApiError("invalid_theme", "Theme package is not valid base64.") from exc
        if len(payload) > MAX_JSON_FILE_BYTES:
            raise ApiError("file_size", "Theme package exceeds 64 MB.", 413)
        with zipfile.ZipFile(io.BytesIO(payload)) as archive:
            roots = {Path(name).parts[0] for name in archive.namelist() if Path(name).parts}
            if len(roots) != 1:
                raise ApiError("invalid_theme", "Theme package must contain one top-level theme directory.")
            theme_id = roots.pop()
            if not THEME_ID.fullmatch(theme_id) or theme_id in {"shaer_pi_os", "shaer_companion", "shaer_backend"}:
                raise ApiError("invalid_theme", "Theme package has an invalid identifier.")
            target = self.root / theme_id
            if target.exists():
                raise ApiError("theme_exists", "A theme with this identifier is already installed.", 409)
            self._validate_archive(archive, 512, 256 * 1024 * 1024)
            if any(Path(member.filename).suffix.lower() in {".js", ".mjs", ".wasm"} for member in archive.infolist()):
                raise ApiError("invalid_theme", "Imported themes must be data-only and cannot contain executable code.")
            with tempfile.TemporaryDirectory(prefix="shaer-theme-", dir=self.root.parent) as temporary:
                staging_root = Path(temporary)
                self._extract_archive(archive, staging_root)
                staged = staging_root / theme_id
                if not (staged / "index.html").is_file():
                    raise ApiError("invalid_theme", "Theme package has no index.html.")
                os.replace(staged, target)
        return self.envelope({"id": theme_id, "installed": True})

    def export_theme(self, theme_id: str) -> dict[str, object]:
        target = self.root / theme_id
        if not THEME_ID.fullmatch(theme_id) or not (target / "index.html").exists():
            raise ApiError("theme_not_found", "Theme does not exist.", 404)
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in target.rglob("*"):
                if path.is_file():
                    archive.write(path, path.relative_to(self.root))
        return self.envelope({"filename": f"{theme_id}.shaer-theme.zip", "content_base64": base64.b64encode(buffer.getvalue()).decode("ascii")})

    def delete_theme(self, theme_id: str) -> dict[str, object]:
        if theme_id in BUILT_IN_THEMES:
            raise ApiError("protected_theme", "Built-in themes cannot be deleted.", 409)
        target = self.root / theme_id
        if not THEME_ID.fullmatch(theme_id) or not target.exists() or target.is_symlink():
            raise ApiError("theme_not_found", "Theme does not exist.", 404)
        shutil.rmtree(target)
        return self.envelope({"id": theme_id, "deleted": True})

    def create_backup(self, passphrase: str, include: list[str] | None = None) -> dict[str, object]:
        if len(passphrase) < 8:
            raise ApiError("weak_passphrase", "Backup passphrase must be at least 8 characters.")
        selected = set(include or ["settings", "library", "themes", "artwork", "recordings"])
        buffer = io.BytesIO()
        manifest = {"format": "shaer-backup-v1", "created_at": int(time.time()), "includes": sorted(selected)}
        with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
            archive.writestr("manifest.json", json.dumps(manifest, sort_keys=True))
            if "settings" in selected and self.settings_store.path.exists():
                archive.write(self.settings_store.path, "settings/settings.json")
            if "library" in selected and self.library_path.exists():
                archive.write(self.library_path, "library/library.db")
            if "music" in selected:
                for path in self.music_root.rglob("*"):
                    if path.is_file():
                        archive.write(path, Path("music") / path.relative_to(self.music_root))
            if "themes" in selected:
                for theme in self.root.glob("shaer_*"):
                    if theme.is_dir() and (theme / "index.html").exists() and theme.name not in {"shaer_companion", "shaer_pi_os"}:
                        for path in theme.rglob("*"):
                            if path.is_file():
                                archive.write(path, Path("themes") / theme.name / path.relative_to(theme))
            artwork_root = self.config_dir / "artwork"
            if "artwork" in selected and artwork_root.exists():
                for path in artwork_root.rglob("*"):
                    if path.is_file():
                        archive.write(path, Path("artwork") / path.relative_to(artwork_root))
            if "recordings" in selected:
                if self.recordings_db.exists():
                    archive.writestr("recordings/recordings.db", self._sqlite_snapshot(self.recordings_db))
                if self.recordings_root.exists():
                    for path in self.recordings_root.rglob("*"):
                        if path.is_file():
                            archive.write(path, Path("recordings/files") / path.relative_to(self.recordings_root))
        encrypted = self._encrypt(buffer.getvalue(), passphrase)
        self.backup_dir.mkdir(parents=True, exist_ok=True)
        name = f"shaer-{time.strftime('%Y%m%d-%H%M%S')}.shaer-backup"
        self._atomic_bytes(self.backup_dir / name, encrypted)
        return self.envelope({"filename": name, "size": len(encrypted), "content_base64": base64.b64encode(encrypted).decode("ascii"), "includes": sorted(selected)})

    def restore_backup(self, content_base64: str, passphrase: str, include: list[str] | None = None) -> dict[str, object]:
        try:
            encrypted = base64.b64decode(content_base64, validate=True)
            if len(encrypted) > MAX_BACKUP_BYTES:
                raise ApiError("backup_too_large", "Backup exceeds the 256 MB encrypted limit.", 413)
            payload = self._decrypt(encrypted, passphrase)
            if len(payload) > MAX_ARCHIVE_EXPANDED_BYTES:
                raise ApiError("backup_too_large", "Decoded backup exceeds the expanded size limit.", 413)
        except ApiError:
            raise
        except (ValueError, TypeError) as exc:
            raise ApiError("invalid_backup", "Backup cannot be decoded or authenticated.") from exc
        selected = set(include or ["settings", "library", "music", "artwork", "recordings"])
        allowed_components = {"settings", "library", "music", "artwork", "recordings"}
        if not selected <= allowed_components:
            raise ApiError("invalid_backup_selection", "Theme and unknown component restore is disabled on this build.")
        restored: list[str] = []
        with zipfile.ZipFile(io.BytesIO(payload)) as archive:
            self._validate_archive(archive, MAX_ARCHIVE_MEMBERS, MAX_ARCHIVE_EXPANDED_BYTES)
            names = set(archive.namelist())
            if "manifest.json" not in names:
                raise ApiError("invalid_backup", "Backup manifest is missing.")
            try:
                manifest = json.loads(archive.read("manifest.json"))
            except (json.JSONDecodeError, UnicodeDecodeError) as exc:
                raise ApiError("invalid_backup", "Backup manifest is invalid.") from exc
            if not isinstance(manifest, dict) or manifest.get("format") != "shaer-backup-v1":
                raise ApiError("invalid_backup", "Backup format is not supported.")

            settings_payload: dict[str, object] | None = None
            library_payload: bytes | None = None
            recordings_payload: bytes | None = None
            if "settings" in selected and "settings/settings.json" in names:
                try:
                    candidate = json.loads(archive.read("settings/settings.json"))
                except (json.JSONDecodeError, UnicodeDecodeError) as exc:
                    raise ApiError("invalid_backup", "Backup settings are invalid.") from exc
                settings_payload = self._validate_settings_document(candidate)
            if "library" in selected and "library/library.db" in names:
                library_payload = archive.read("library/library.db")
                self._validate_sqlite_payload(library_payload, {"schema_migrations", "tracks", "playlists", "playlist_tracks"})
            if "recordings" in selected and "recordings/recordings.db" in names:
                recordings_payload = archive.read("recordings/recordings.db")
                self._validate_sqlite_payload(recordings_payload, {"schema_migrations", "recordings"})

            # Stage every selected component beside its destination. No live path is
            # changed until all staging succeeds, and commit can roll every swap back.
            replacements: list[tuple[Path, Path]] = []
            staging_roots: list[Path] = []
            try:
                if settings_payload is not None:
                    staged, root = self._stage_restore_bytes(
                        self.settings_store.path,
                        json.dumps(settings_payload, indent=2, sort_keys=True).encode("utf-8"),
                        0o600,
                    )
                    replacements.append((self.settings_store.path, staged))
                    staging_roots.append(root)
                    restored.append("settings")
                if library_payload is not None:
                    target = self.library_path.with_suffix(".db.restore-pending")
                    staged, root = self._stage_restore_bytes(target, library_payload, 0o600)
                    replacements.append((target, staged))
                    staging_roots.append(root)
                    restored.append("library")
                if "music" in selected:
                    staged_tree = self._stage_archive_tree(archive, names, ("music",), self.music_root)
                    if staged_tree:
                        staged, root = staged_tree
                        replacements.append((self.music_root, staged))
                        staging_roots.append(root)
                        restored.append("music")
                if "artwork" in selected:
                    artwork_root = self.config_dir / "artwork"
                    staged_tree = self._stage_archive_tree(archive, names, ("artwork",), artwork_root)
                    if staged_tree:
                        staged, root = staged_tree
                        replacements.append((artwork_root, staged))
                        staging_roots.append(root)
                        restored.append("artwork")
                if "recordings" in selected:
                    if recordings_payload is not None:
                        target = self.recordings_db.with_suffix(".db.restore-pending")
                        staged, root = self._stage_restore_bytes(target, recordings_payload, 0o600)
                        replacements.append((target, staged))
                        staging_roots.append(root)
                    staged_tree = self._stage_archive_tree(archive, names, ("recordings", "files"), self.recordings_root)
                    if staged_tree:
                        staged, root = staged_tree
                        replacements.append((self.recordings_root, staged))
                        staging_roots.append(root)
                    if recordings_payload is not None or staged_tree:
                        restored.append("recordings")
                self._commit_restore(replacements)
            finally:
                for root in staging_roots:
                    self._remove_path(root)
        return self.envelope({"restored": restored, "reboot_required": bool({"library", "recordings"} & set(restored))})

    def stage_update(self, manifest: Mapping[str, object], content_base64: str) -> dict[str, object]:
        version = str(manifest.get("version") or "").strip()
        expected = str(manifest.get("sha256") or "").lower()
        signature = str(manifest.get("signature_base64") or "")
        if not re.fullmatch(r"[0-9A-Za-z._-]{1,40}", version) or not re.fullmatch(r"[0-9a-f]{64}", expected):
            raise ApiError("invalid_update", "Update manifest is invalid.")
        try:
            payload = base64.b64decode(content_base64, validate=True)
        except (ValueError, TypeError) as exc:
            raise ApiError("invalid_update", "Update payload is not valid base64.") from exc
        if hashlib.sha256(payload).hexdigest() != expected:
            raise ApiError("checksum_failed", "Firmware checksum does not match.", 422)
        public_key = os.environ.get("SHAER_UPDATE_PUBLIC_KEY")
        if public_key:
            self._verify_signature(Path(public_key), payload, signature)
        elif not bool(manifest.get("development_unsigned")):
            raise ApiError("signature_required", "No update signing key is configured.", 503)
        self.update_dir.mkdir(parents=True, exist_ok=True)
        package = self.update_dir / f"shaer-{version}.update"
        self._atomic_bytes(package, payload)
        status = {"state": "staged", "version": version, "package": str(package), "progress": 100, "verified": True}
        JsonStore(self.update_dir / "status.json", {}).save(status)
        return self.envelope(status)

    def update_status(self) -> dict[str, object]:
        return self.envelope(JsonStore(self.update_dir / "status.json", {"state": "idle", "progress": 0}).load())

    def install_update(self, rollback: bool = False) -> dict[str, object]:
        helper = os.environ.get("SHAER_UPDATE_HELPER")
        if not helper:
            raise ApiError("update_helper_unavailable", "The privileged update helper is not configured on this device.", 503)
        action = "rollback" if rollback else "install"
        result = subprocess.run([helper, action], capture_output=True, text=True, timeout=30, check=False)
        if result.returncode != 0:
            raise ApiError("update_failed", result.stderr.strip() or "Update helper failed.", 500)
        return self.envelope({"state": action + "ing", "reboot_required": True, "output": result.stdout[-2000:]})

    def _database(self):
        from shaer_music.library import LibraryDatabase

        database = LibraryDatabase(self.library_path)
        database.migrate()
        return _DatabaseContext(database)

    @staticmethod
    def _audio_signature_matches(suffix: str, payload: bytes) -> bool:
        signatures = {
            ".flac": payload.startswith(b"fLaC"),
            ".wav": len(payload) >= 12 and payload.startswith(b"RIFF") and payload[8:12] == b"WAVE",
            ".ogg": payload.startswith(b"OggS"),
            ".m4a": len(payload) >= 12 and payload[4:8] == b"ftyp",
            ".aac": len(payload) >= 2 and payload[0] == 0xFF and payload[1] & 0xF6 == 0xF0,
            ".mp3": payload.startswith(b"ID3") or (len(payload) >= 2 and payload[0] == 0xFF and payload[1] & 0xE0 == 0xE0),
        }
        return bool(signatures.get(suffix, False))

    @staticmethod
    def _validated_track_ids(connection: sqlite3.Connection, track_ids: list[int]) -> list[int]:
        try:
            normalized = [int(track_id) for track_id in track_ids]
        except (TypeError, ValueError) as exc:
            raise ApiError("invalid_track_ids", "Playlist track identifiers must be integers.") from exc
        if len(normalized) != len(set(normalized)):
            raise ApiError("invalid_track_ids", "A playlist cannot contain duplicate track identifiers.")
        if not normalized:
            return []
        placeholders = ",".join("?" for _ in normalized)
        existing = {int(row[0]) for row in connection.execute(f"SELECT id FROM tracks WHERE id IN ({placeholders})", normalized)}
        missing = [track_id for track_id in normalized if track_id not in existing]
        if missing:
            raise ApiError("invalid_track_ids", f"Unknown track identifiers: {missing}", 422)
        return normalized

    @staticmethod
    def _validate_setting_value(category: str, key: str, value: object, default: object) -> None:
        if isinstance(default, bool):
            valid = isinstance(value, bool)
        elif isinstance(default, int):
            valid = isinstance(value, int) and not isinstance(value, bool)
        elif isinstance(default, float):
            valid = isinstance(value, (int, float)) and not isinstance(value, bool)
        elif isinstance(default, str):
            valid = isinstance(value, str) and len(value) <= 128
        elif isinstance(default, list):
            valid = isinstance(value, list) and len(value) <= 64
        else:
            valid = type(value) is type(default)
        if not valid:
            raise ApiError("invalid_setting", f"Invalid value for {category}.{key}.")
        ranges = {
            ("audio", "volume_percent"): (0, 100), ("audio", "volume_limit"): (0, 100), ("audio", "bass"): (-12, 12),
            ("audio", "mid"): (-12, 12), ("audio", "treble"): (-12, 12),
            ("audio", "crossfade_s"): (0, 30), ("display", "brightness"): (0, 100),
            ("display", "sleep_timeout_s"): (0, 86400), ("display", "animation_speed"): (0.25, 3),
            ("power", "auto_shutdown_min"): (0, 1440), ("power", "low_battery_percent"): (1, 50),
            ("library", "artwork_cache_mb"): (16, 4096),
        }
        if (category, key) in ranges:
            minimum, maximum = ranges[(category, key)]
            if not minimum <= float(value) <= maximum:  # type: ignore[arg-type]
                raise ApiError("invalid_setting", f"{category}.{key} is outside the supported range.")

    @classmethod
    def _validate_settings_document(cls, payload: object) -> dict[str, object]:
        if not isinstance(payload, dict):
            raise ApiError("invalid_backup", "Backup settings are invalid.")
        for category, values in payload.items():
            defaults = DEFAULT_SETTINGS.get(category)
            if not isinstance(defaults, dict) or not isinstance(values, dict):
                raise ApiError("invalid_backup", f"Backup contains an unknown settings category: {category}")
            for key, value in values.items():
                if key not in defaults:
                    raise ApiError("invalid_backup", f"Backup contains an unknown setting: {category}.{key}")
                cls._validate_setting_value(category, key, value, defaults[key])
        return payload

    @staticmethod
    def _validate_sqlite_payload(payload: bytes, required_tables: set[str]) -> None:
        if not payload or len(payload) > MAX_BACKUP_BYTES:
            raise ApiError("invalid_backup", "Backup database has an invalid size.")
        with tempfile.TemporaryDirectory(prefix="shaer-db-check-") as temporary:
            candidate = Path(temporary) / "candidate.db"
            candidate.write_bytes(payload)
            try:
                connection = sqlite3.connect(f"file:{candidate}?mode=ro", uri=True)
                try:
                    result = connection.execute("PRAGMA quick_check").fetchone()
                    tables = {str(row[0]) for row in connection.execute("SELECT name FROM sqlite_master WHERE type='table'")}
                finally:
                    connection.close()
            except sqlite3.DatabaseError as exc:
                raise ApiError("invalid_backup", "Backup contains a corrupt SQLite database.") from exc
        if not result or result[0] != "ok" or not required_tables <= tables:
            raise ApiError("invalid_backup", "Backup database schema or integrity check failed.")

    @staticmethod
    def _validate_archive(archive: zipfile.ZipFile, maximum_members: int, maximum_expanded: int) -> None:
        members = archive.infolist()
        if len(members) > maximum_members:
            raise ApiError("archive_limit", "Archive contains too many files.", 413)
        expanded = 0
        seen: set[str] = set()
        for member in members:
            name = member.filename
            path = Path(name)
            normalized = path.as_posix().rstrip("/")
            if not normalized or normalized in seen or name.startswith(("/", "\\")) or "\\" in name or ".." in path.parts or len(path.parts) > 16 or len(name) > 240:
                raise ApiError("invalid_archive", "Archive contains an unsafe or duplicate path.")
            seen.add(normalized)
            mode = member.external_attr >> 16
            file_type = stat.S_IFMT(mode)
            if file_type and not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
                raise ApiError("invalid_archive", "Archive contains a link or special file.")
            if member.flag_bits & 0x1:
                raise ApiError("invalid_archive", "Encrypted ZIP members are not supported.")
            expanded += member.file_size
            if expanded > maximum_expanded:
                raise ApiError("archive_limit", "Archive expands beyond the allowed size.", 413)
            if member.file_size and (member.compress_size == 0 or member.file_size / member.compress_size > MAX_ARCHIVE_RATIO):
                raise ApiError("archive_limit", "Archive compression ratio is unsafe.", 413)

    @staticmethod
    def _extract_archive(archive: zipfile.ZipFile, destination: Path) -> None:
        for member in archive.infolist():
            target = destination.joinpath(*Path(member.filename).parts)
            if member.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(member) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output, length=1024 * 1024)

    @staticmethod
    def _stage_restore_bytes(target: Path, payload: bytes, mode: int) -> tuple[Path, Path]:
        target.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        root = Path(tempfile.mkdtemp(prefix=".shaer-restore-", dir=target.parent))
        staged = root / target.name
        try:
            staged.write_bytes(payload)
            staged.chmod(mode)
            return staged, root
        except Exception:
            CompanionService._remove_path(root)
            raise

    @staticmethod
    def _stage_archive_tree(
        archive: zipfile.ZipFile,
        names: set[str],
        prefix: tuple[str, ...],
        target: Path,
    ) -> tuple[Path, Path] | None:
        matching = [
            name for name in names
            if not name.endswith("/") and len(Path(name).parts) > len(prefix) and Path(name).parts[:len(prefix)] == prefix
        ]
        if not matching:
            return None
        target.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        root = Path(tempfile.mkdtemp(prefix=".shaer-restore-", dir=target.parent))
        staged = root / target.name
        staged.mkdir(mode=0o700)
        try:
            for name in sorted(matching):
                relative = Path(*Path(name).parts[len(prefix):])
                destination = staged / relative
                if not CompanionService._is_within(destination, staged):
                    raise ApiError("invalid_backup", "Backup contains an unsafe restore path.")
                destination.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(name) as source, destination.open("wb") as output:
                    shutil.copyfileobj(source, output, length=1024 * 1024)
            return staged, root
        except Exception:
            CompanionService._remove_path(root)
            raise

    @staticmethod
    def _commit_restore(replacements: list[tuple[Path, Path]]) -> None:
        committed: list[tuple[Path, Path | None]] = []
        token = secrets.token_hex(8)
        try:
            for target, staged in replacements:
                backup: Path | None = None
                if target.exists() or target.is_symlink():
                    backup = target.parent / f".{target.name}.shaer-rollback-{token}"
                    os.replace(target, backup)
                try:
                    os.replace(staged, target)
                except Exception:
                    if backup is not None:
                        os.replace(backup, target)
                    raise
                committed.append((target, backup))
        except Exception as exc:
            for target, backup in reversed(committed):
                CompanionService._remove_path(target)
                if backup is not None and (backup.exists() or backup.is_symlink()):
                    os.replace(backup, target)
            raise ApiError("restore_failed", "Backup restore was rolled back without changing live data.", 500) from exc
        for _target, backup in committed:
            if backup is not None:
                CompanionService._remove_path(backup)

    @staticmethod
    def _remove_path(path: Path) -> None:
        if path.is_symlink() or path.is_file():
            path.unlink(missing_ok=True)
        elif path.exists():
            shutil.rmtree(path)

    @staticmethod
    def _sqlite_snapshot(path: Path) -> bytes:
        with tempfile.TemporaryDirectory(prefix="shaer-sqlite-backup-") as temporary:
            target = Path(temporary) / "snapshot.db"
            source_connection = sqlite3.connect(path)
            target_connection = sqlite3.connect(target)
            try:
                source_connection.backup(target_connection)
            finally:
                target_connection.close()
                source_connection.close()
            return target.read_bytes()

    def _index_library(self) -> dict[str, int]:
        from shaer_music.library import LibraryDatabase, LibraryIndexer

        database = LibraryDatabase(self.library_path)
        try:
            database.migrate()
            return LibraryIndexer(database, self.music_root, self.config_dir / "artwork").index()
        finally:
            database.close()

    @staticmethod
    def _is_within(path: Path, root: Path) -> bool:
        try:
            path.expanduser().resolve().relative_to(root.expanduser().resolve())
            return True
        except ValueError:
            return False

    @staticmethod
    def _unique_path(root: Path, filename: str) -> Path:
        candidate = root / filename
        stem, suffix = candidate.stem, candidate.suffix
        counter = 2
        while candidate.exists():
            candidate = root / f"{stem} ({counter}){suffix}"
            counter += 1
        return candidate

    @staticmethod
    def _atomic_bytes(target: Path, payload: bytes) -> None:
        target.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix=".shaer-", dir=target.parent)
        try:
            with os.fdopen(fd, "wb") as handle:
                handle.write(payload)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, target)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

    @staticmethod
    def _encrypt(payload: bytes, passphrase: str) -> bytes:
        salt, nonce = secrets.token_bytes(16), secrets.token_bytes(16)
        key = hashlib.scrypt(passphrase.encode("utf-8"), salt=salt, n=2**14, r=8, p=1, dklen=64)
        stream = CompanionService._keystream(key[:32], nonce, len(payload))
        ciphertext = bytes(left ^ right for left, right in zip(payload, stream))
        header = b"SHAERBACKUP1" + salt + nonce
        tag = hmac.new(key[32:], header + ciphertext, hashlib.sha256).digest()
        return header + tag + ciphertext

    @staticmethod
    def _decrypt(payload: bytes, passphrase: str) -> bytes:
        if len(payload) < 76 or not payload.startswith(b"SHAERBACKUP1"):
            raise ValueError("Invalid backup header")
        salt, nonce, tag, ciphertext = payload[12:28], payload[28:44], payload[44:76], payload[76:]
        key = hashlib.scrypt(passphrase.encode("utf-8"), salt=salt, n=2**14, r=8, p=1, dklen=64)
        expected = hmac.new(key[32:], payload[:44] + ciphertext, hashlib.sha256).digest()
        if not hmac.compare_digest(tag, expected):
            raise ValueError("Backup authentication failed")
        stream = CompanionService._keystream(key[:32], nonce, len(ciphertext))
        return bytes(left ^ right for left, right in zip(ciphertext, stream))

    @staticmethod
    def _keystream(key: bytes, nonce: bytes, length: int) -> bytes:
        output = bytearray()
        counter = 0
        while len(output) < length:
            output.extend(hmac.new(key, nonce + counter.to_bytes(8, "big"), hashlib.sha256).digest())
            counter += 1
        return bytes(output[:length])

    @staticmethod
    def _verify_signature(public_key: Path, payload: bytes, signature_base64: str) -> None:
        try:
            signature = base64.b64decode(signature_base64, validate=True)
        except (ValueError, TypeError) as exc:
            raise ApiError("signature_failed", "Update signature is invalid.", 422) from exc
        with tempfile.TemporaryDirectory(prefix="shaer-update-") as temporary:
            package = Path(temporary) / "package"
            signed = Path(temporary) / "signature"
            package.write_bytes(payload)
            signed.write_bytes(signature)
            result = subprocess.run(
                ["openssl", "dgst", "-sha256", "-verify", str(public_key), "-signature", str(signed), str(package)],
                capture_output=True, text=True, check=False,
            )
        if result.returncode != 0:
            raise ApiError("signature_failed", "Firmware signature verification failed.", 422)

    @staticmethod
    def _temperature() -> float | None:
        path = Path("/sys/class/thermal/thermal_zone0/temp")
        try:
            return round(int(path.read_text(encoding="utf-8").strip()) / 1000, 1)
        except (OSError, ValueError):
            return None

    @staticmethod
    def _wifi_signal() -> int | None:
        try:
            lines = Path("/proc/net/wireless").read_text(encoding="utf-8").splitlines()[2:]
            if not lines:
                return None
            fields = lines[0].replace(".", "").split()
            return int(float(fields[3]))
        except (OSError, ValueError, IndexError):
            return None

    @staticmethod
    def _capability(state: str, reason: str | None = None) -> dict[str, object]:
        if state not in CAPABILITY_STATES:
            state = "unsupported"
        payload: dict[str, object] = {"state": state}
        if reason:
            payload["reason"] = reason
        return payload


class _DatabaseContext:
    def __init__(self, database: object):
        self.database = database

    def __enter__(self) -> sqlite3.Connection:
        return self.database.connection  # type: ignore[attr-defined]

    def __exit__(self, exc_type, exc, traceback) -> None:
        self.database.close()  # type: ignore[attr-defined]
