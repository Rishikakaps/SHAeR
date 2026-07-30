#!/usr/bin/env python3
"""Small CLI for authenticating a running SHAeR Pi OS server."""

from __future__ import annotations

import argparse
import base64
import json
import urllib.error
import urllib.request
from pathlib import Path


def request_json(url: str, method: str = "GET") -> dict[str, object]:
    request = urllib.request.Request(url, data=b"{}" if method == "POST" else None, method=method)
    if method == "POST":
        request.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        payload = json.loads(exc.read().decode("utf-8"))
        raise SystemExit(str(payload.get("error") or f"HTTP {exc.code}")) from exc


def main() -> None:
    parser = argparse.ArgumentParser(description="Configure Spotify for SHAeR.")
    parser.add_argument("command", choices=("status", "login", "logout"))
    parser.add_argument("--server", default="http://127.0.0.1:8775")
    parser.add_argument("--qr", type=Path, help="Write the login QR code to this PNG file.")
    args = parser.parse_args()
    base = args.server.rstrip("/")
    if args.command == "status":
        print(json.dumps(request_json(f"{base}/api/spotify/status"), indent=2))
        return
    if args.command == "logout":
        print(json.dumps(request_json(f"{base}/api/spotify/logout", method="POST"), indent=2))
        return
    payload = request_json(f"{base}/api/spotify/login?launch=1")
    print("Open this URL if the browser did not launch:")
    print(payload["authorization_url"])
    qr = payload.get("qr_data_uri")
    if args.qr and isinstance(qr, str) and "," in qr:
        args.qr.write_bytes(base64.b64decode(qr.split(",", 1)[1]))
        print(f"QR code written to {args.qr}")
    elif args.qr:
        print("QR generation is unavailable; install python3-qrcode.")


if __name__ == "__main__":
    main()

