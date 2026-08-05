"""Centralized Spotify Web API client for SHAeR."""

from __future__ import annotations

import json
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Callable, Mapping

from .spotify_auth import SpotifyAuthError, SpotifyAuthManager


API_BASE = "https://api.spotify.com/v1"


class SpotifyApiError(RuntimeError):
    def __init__(self, status: int, message: str, reason: str | None = None):
        super().__init__(message)
        self.status = status
        self.reason = reason


Transport = Callable[[str, str, Mapping[str, str], bytes | None], tuple[int, Mapping[str, str], bytes]]


def _transport(method: str, url: str, headers: Mapping[str, str], body: bytes | None):
    request = urllib.request.Request(url, data=body, headers=dict(headers), method=method)
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            return response.status, dict(response.headers.items()), response.read()
    except urllib.error.HTTPError as exc:
        return exc.code, dict(exc.headers.items()), exc.read()
    except (urllib.error.URLError, TimeoutError) as exc:
        raise SpotifyApiError(0, "Spotify is temporarily unreachable.") from exc


class SpotifyClient:
    def __init__(
        self,
        auth: SpotifyAuthManager,
        transport: Transport = _transport,
        sleep: Callable[[float], None] = time.sleep,
    ):
        self.auth = auth
        self._transport = transport
        self._sleep = sleep

    def request(
        self,
        method: str,
        path: str,
        params: Mapping[str, object] | None = None,
        json_body: object | None = None,
    ) -> object | None:
        query = urllib.parse.urlencode(
            {key: value for key, value in (params or {}).items() if value is not None}, doseq=True
        )
        url = path if path.startswith("https://") else f"{API_BASE}{path}"
        if query:
            url = f"{url}{'&' if '?' in url else '?'}{query}"
        body = json.dumps(json_body).encode("utf-8") if json_body is not None else None
        refreshed = False
        retries = 0
        while True:
            try:
                access_token = self.auth.access_token(force_refresh=refreshed)
            except SpotifyAuthError:
                raise
            headers = {"Authorization": f"Bearer {access_token}", "Accept": "application/json"}
            if body is not None:
                headers["Content-Type"] = "application/json"
            status, response_headers, payload = self._transport(method, url, headers, body)
            if status == 401 and not refreshed:
                refreshed = True
                continue
            if status == 429 and retries < 3:
                retries += 1
                delay = min(30, max(1, int(response_headers.get("Retry-After", "1"))))
                self._sleep(delay)
                continue
            if not 200 <= status < 300:
                message = f"Spotify API request failed with HTTP {status}."
                reason = None
                try:
                    error_payload = json.loads(payload.decode("utf-8"))
                    detail = error_payload.get("error", {})
                    if isinstance(detail, dict) and detail.get("message"):
                        message = str(detail["message"])
                        reason = str(detail.get("reason")) if detail.get("reason") else None
                except (UnicodeDecodeError, json.JSONDecodeError, AttributeError):
                    pass
                raise SpotifyApiError(status, message, reason)
            if not payload:
                return None
            try:
                return json.loads(payload.decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError):
                return payload.decode("utf-8", errors="replace")

    def current_user(self):
        return self.request("GET", "/me")

    def user_profile(self):
        return self.current_user()

    def saved_tracks(self, limit: int = 50, offset: int = 0):
        return self.request("GET", "/me/tracks", {"limit": limit, "offset": offset})

    def saved_albums(self, limit: int = 50, offset: int = 0):
        return self.request("GET", "/me/albums", {"limit": limit, "offset": offset})

    def saved_artists(self, limit: int = 50, after: str | None = None):
        return self.request("GET", "/me/following", {"type": "artist", "limit": limit, "after": after})

    def playlists(self, limit: int = 50, offset: int = 0):
        return self.request("GET", "/me/playlists", {"limit": limit, "offset": offset})

    def playlist_items(self, playlist_id: str, limit: int = 50, offset: int = 0):
        if not playlist_id or any(character not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" for character in playlist_id):
            raise ValueError("A valid Spotify playlist ID is required.")
        return self.request("GET", f"/playlists/{playlist_id}/items", {"limit": limit, "offset": offset})

    def recently_played(self, limit: int = 50, after: int | None = None):
        return self.request("GET", "/me/player/recently-played", {"limit": limit, "after": after})

    def devices(self):
        return self.request("GET", "/me/player/devices")

    def playback_state(self):
        return self.request("GET", "/me/player")

    def queue(self):
        return self.request("GET", "/me/player/queue")

    def search(self, query: str, types: tuple[str, ...] = ("track", "album", "artist", "playlist"), limit: int = 10):
        return self.request("GET", "/search", {"q": query, "type": ",".join(types), "limit": limit})

    def transfer(self, device_id: str, play: bool = False):
        return self.request("PUT", "/me/player", json_body={"device_ids": [device_id], "play": play})

    def resume(self, device_id: str | None = None):
        return self.request("PUT", "/me/player/play", {"device_id": device_id})

    def play_uris(self, uris: list[str], device_id: str | None = None):
        return self.request("PUT", "/me/player/play", {"device_id": device_id}, {"uris": uris})

    def play_context(self, context_uri: str, device_id: str | None = None):
        return self.request("PUT", "/me/player/play", {"device_id": device_id}, {"context_uri": context_uri})

    def pause(self, device_id: str | None = None):
        return self.request("PUT", "/me/player/pause", {"device_id": device_id})

    def next(self, device_id: str | None = None):
        return self.request("POST", "/me/player/next", {"device_id": device_id})

    def previous(self, device_id: str | None = None):
        return self.request("POST", "/me/player/previous", {"device_id": device_id})

    def seek(self, position_ms: int, device_id: str | None = None):
        return self.request("PUT", "/me/player/seek", {"position_ms": max(0, position_ms), "device_id": device_id})

    def volume(self, percent: int, device_id: str | None = None):
        return self.request("PUT", "/me/player/volume", {"volume_percent": min(100, max(0, percent)), "device_id": device_id})

    def shuffle(self, enabled: bool, device_id: str | None = None):
        return self.request("PUT", "/me/player/shuffle", {"state": str(bool(enabled)).lower(), "device_id": device_id})

    def repeat(self, mode: str, device_id: str | None = None):
        if mode not in {"off", "context", "track"}:
            raise ValueError("Unsupported repeat mode.")
        return self.request("PUT", "/me/player/repeat", {"state": mode, "device_id": device_id})

    def add_to_queue(self, uri: str, device_id: str | None = None):
        return self.request("POST", "/me/player/queue", {"uri": uri, "device_id": device_id})
