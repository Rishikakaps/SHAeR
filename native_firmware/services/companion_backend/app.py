#!/usr/bin/env python3
"""Minimal companion backend API for early SHAeR development."""

from __future__ import annotations

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SETTINGS_PATH = ROOT / "configs" / "settings.json"


def load_settings() -> dict:
    return json.loads(SETTINGS_PATH.read_text())


def save_settings(settings: dict) -> None:
    SETTINGS_PATH.write_text(json.dumps(settings, indent=2) + "\n")


class Handler(BaseHTTPRequestHandler):
    def _json(self, status: int, body: dict) -> None:
        data = json.dumps(body, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        if self.path == "/api/v1/settings":
            self._json(200, load_settings())
            return
        self._json(404, {"error": "not_found"})

    def do_PUT(self) -> None:
        if not self.path.startswith("/api/v1/settings/"):
            self._json(404, {"error": "not_found"})
            return

        key = self.path.removeprefix("/api/v1/settings/")
        length = int(self.headers.get("content-length", "0"))
        body = json.loads(self.rfile.read(length) or b"{}")
        settings = load_settings()

        cursor = settings
        parts = key.split(".")
        for part in parts[:-1]:
            cursor = cursor.setdefault(part, {})
        cursor[parts[-1]] = body.get("value")
        save_settings(settings)
        self._json(200, {"updated": key, "value": body.get("value")})


if __name__ == "__main__":
    print("[companion] http://127.0.0.1:8780")
    ThreadingHTTPServer(("127.0.0.1", 8780), Handler).serve_forever()

