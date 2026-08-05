"""Resilient background workers for Spotify state on SHAeR."""

from __future__ import annotations

import threading
import time
from collections.abc import Callable

from .spotify_auth import SpotifyAuthError, SpotifyAuthManager
from .spotify_client import SpotifyApiError, SpotifyClient
from .spotify_connect import LibrespotManager


class SpotifyServices:
    def __init__(
        self,
        auth: SpotifyAuthManager,
        client: SpotifyClient,
        connect: LibrespotManager,
        on_playback: Callable[[object | None], None] | None = None,
        interval_s: float = 5.0,
    ):
        self.auth = auth
        self.client = client
        self.connect = connect
        self.on_playback = on_playback
        self.interval_s = interval_s
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="shaer-spotify-services", daemon=True)
        self._thread.start()

    def stop(self, timeout: float = 2.0) -> None:
        self._stop.set()
        if self._thread:
            self._thread.join(timeout)

    def tick(self) -> None:
        status = self.auth.status()
        if not status["authenticated"]:
            return
        self.auth.access_token()
        playback = self.client.playback_state()
        if self.on_playback:
            self.on_playback(playback)
        if self.connect.installed() and not self.connect.service_active():
            self.connect.start()

    def _run(self) -> None:
        backoff = self.interval_s
        while not self._stop.is_set():
            try:
                self.tick()
                backoff = self.interval_s
            except (SpotifyAuthError, SpotifyApiError, OSError, RuntimeError):
                backoff = min(60.0, max(self.interval_s, backoff * 2))
            self._stop.wait(backoff)

