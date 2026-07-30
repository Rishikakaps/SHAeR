"""HTTP-facing Spotify runtime used by the SHAeR Pi UI server."""

from __future__ import annotations

import base64
import io
import os
from pathlib import Path
from typing import Mapping

from shaer_music import (
    LibrespotManager,
    SpotifyApiError,
    SpotifyAuthError,
    SpotifyAuthManager,
    SpotifyCache,
    SpotifyClient,
    TokenStore,
    spotify_playback_state,
)


class SpotifyRuntime:
    def __init__(self, port: int):
        client_id = os.environ.get("SPOTIFY_CLIENT_ID", "").strip()
        self.configured = bool(client_id)
        self.auth: SpotifyAuthManager | None = None
        self.client: SpotifyClient | None = None
        self.connect: LibrespotManager | None = None
        self.cache: SpotifyCache | None = None
        if not self.configured:
            return
        redirect_uri = os.environ.get(
            "SPOTIFY_REDIRECT_URI", f"http://127.0.0.1:{port}/api/spotify/callback"
        )
        config_root = Path(os.environ.get("SHAER_CONFIG_DIR", "~/.config/shaer")).expanduser()
        cache_root = Path(os.environ.get("SHAER_CACHE_DIR", "~/.cache/shaer/spotify")).expanduser()
        self.auth = SpotifyAuthManager(
            client_id=client_id,
            redirect_uri=redirect_uri,
            token_store=TokenStore(config_root / "spotify-token.json"),
        )
        self.client = SpotifyClient(self.auth)
        self.connect = LibrespotManager(self.client)
        self.cache = SpotifyCache(cache_root)

    def status(self) -> dict[str, object]:
        if not self.configured or self.auth is None:
            return {"configured": False, "authenticated": False, "login_pending": False, "expires_at": None}
        return self.auth.status()

    def begin_login(self, launch_browser: bool = True, timeout_s: int = 300) -> dict[str, object]:
        auth = self._auth()
        attempt = auth.begin_login(timeout_s=timeout_s, launch_browser=launch_browser)
        return {
            "state": attempt.state,
            "authorization_url": attempt.authorization_url,
            "expires_at": int(attempt.expires_at),
            "browser_requested": launch_browser,
            "qr_data_uri": self._qr_data_uri(attempt.authorization_url),
        }

    def callback(self, query: Mapping[str, list[str]]) -> dict[str, object]:
        auth = self._auth()
        state = (query.get("state") or [""])[0]
        code = (query.get("code") or [""])[0] or None
        error = (query.get("error") or [""])[0] or None
        auth.complete_login(state=state, code=code, error=error)
        profile = self._client().current_user()
        display_name = profile.get("display_name") if isinstance(profile, dict) else None
        return {"ok": True, "authenticated": True, "display_name": display_name}

    def cancel(self, state: str | None = None) -> dict[str, object]:
        return {"ok": self._auth().cancel_login(state)}

    def logout(self) -> dict[str, object]:
        if self.connect:
            self.connect.stop()
        self._auth().logout()
        return {"ok": True, "authenticated": False}

    def profile(self):
        return self._client().current_user()

    def library(self, kind: str, query: Mapping[str, list[str]]):
        client = self._client()
        limit = min(50, max(1, int((query.get("limit") or ["20"])[0])))
        offset = max(0, int((query.get("offset") or ["0"])[0]))
        if kind == "tracks":
            return client.saved_tracks(limit, offset)
        if kind == "albums":
            return client.saved_albums(limit, offset)
        if kind == "artists":
            return client.saved_artists(limit, (query.get("after") or [None])[0])
        if kind == "playlists":
            return client.playlists(limit, offset)
        if kind == "recent":
            return client.recently_played(limit)
        raise ValueError("Unknown Spotify library collection.")

    def playlist_items(self, playlist_id: str, query: Mapping[str, list[str]]):
        limit = min(50, max(1, int((query.get("limit") or ["50"])[0])))
        offset = max(0, int((query.get("offset") or ["0"])[0]))
        return self._client().playlist_items(playlist_id, limit, offset)

    def search(self, query: Mapping[str, list[str]]):
        term = (query.get("q") or [""])[0].strip()
        if not term:
            raise ValueError("Search query is required.")
        limit = min(10, max(1, int((query.get("limit") or ["10"])[0])))
        return self._client().search(term, limit=limit)

    def playback(self) -> dict[str, object]:
        payload = self._client().playback_state()
        return spotify_playback_state(payload if isinstance(payload, dict) else None).public_dict()

    def queue(self):
        return self._client().queue()

    def connect_status(self) -> dict[str, object]:
        return self._connect().status().public_dict()

    def connect_transfer(self, play: bool = False) -> dict[str, object]:
        self._connect().transfer(play=play)
        return {"ok": True}

    def control(
        self,
        action: str,
        value: int | None = None,
        uri: str | None = None,
        context_uri: str | None = None,
        enabled: bool | None = None,
        mode: str | None = None,
    ) -> dict[str, object]:
        client = self._client()
        if action == "toggle-play":
            state = client.playback_state()
            if isinstance(state, dict) and state.get("is_playing"):
                client.pause()
            else:
                client.resume()
        elif action == "play":
            client.resume()
        elif action == "pause":
            client.pause()
        elif action == "next":
            client.next()
        elif action == "previous":
            client.previous()
        elif action == "seek":
            client.seek(int(value or 0))
        elif action == "volume":
            client.volume(int(value or 0))
        elif action == "play-uri":
            if not uri or not uri.startswith("spotify:"):
                raise ValueError("A valid Spotify URI is required.")
            client.play_uris([uri])
        elif action == "play-context":
            if not context_uri or not context_uri.startswith("spotify:"):
                raise ValueError("A valid Spotify context URI is required.")
            client.play_context(context_uri)
        elif action == "shuffle":
            client.shuffle(bool(enabled))
        elif action == "repeat":
            client.repeat(str(mode or "off"))
        else:
            raise ValueError("Unsupported playback action.")
        return {"ok": True, "action": action}

    def _auth(self) -> SpotifyAuthManager:
        if self.auth is None:
            raise SpotifyAuthError("Set SPOTIFY_CLIENT_ID before logging in.")
        return self.auth

    def _client(self) -> SpotifyClient:
        if self.client is None:
            raise SpotifyAuthError("Spotify is not configured.")
        return self.client

    def _connect(self) -> LibrespotManager:
        if self.connect is None:
            raise SpotifyAuthError("Spotify Connect is not configured.")
        return self.connect

    @staticmethod
    def _qr_data_uri(value: str) -> str | None:
        try:
            import qrcode  # type: ignore
        except ImportError:
            return None
        output = io.BytesIO()
        image = qrcode.make(value)
        image.save(output, format="PNG")
        return "data:image/png;base64," + base64.b64encode(output.getvalue()).decode("ascii")


__all__ = ["SpotifyApiError", "SpotifyAuthError", "SpotifyRuntime"]
