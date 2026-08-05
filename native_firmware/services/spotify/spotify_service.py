#!/usr/bin/env python3
"""Spotify metadata/auth service starter.

This is an external helper service, not the core app spine. The single C++
SHAeR app owns UI, input, playback state, and recovery behavior. This Python
service will eventually expose Spotify auth/metadata over one stable local IPC
or HTTP boundary.
"""

from __future__ import annotations

import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CONFIG_PATH = ROOT / "configs" / "settings.json"


def read_settings() -> dict:
    if not CONFIG_PATH.exists():
        return {}
    return json.loads(CONFIG_PATH.read_text())


class Handler(BaseHTTPRequestHandler):
    def _send(self, status: int, body: dict) -> None:
        data = json.dumps(body, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        if self.path == "/api/v1/spotify/auth-state":
            spotify = read_settings().get("spotify", {})
            self._send(200, {"state": spotify.get("auth_state", "not_configured")})
            return
        self._send(404, {"error": "not_found"})

    def do_POST(self) -> None:
        if self.path == "/api/v1/spotify/simulate-wifi-loss":
            self._send(200, {"event": "wifi_disconnected_mid_song", "note": "handled inside C++ simulator with spotify_drop"})
            return
        if self.path == "/api/v1/spotify/re-auth":
            self._send(202, {"state": "reauth_started"})
            return
        self._send(404, {"error": "not_found"})

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"[spotify] {fmt % args}")


if __name__ == "__main__":
    server = ThreadingHTTPServer(("127.0.0.1", 8765), Handler)
    print("[spotify] http://127.0.0.1:8765")
    server.serve_forever()
