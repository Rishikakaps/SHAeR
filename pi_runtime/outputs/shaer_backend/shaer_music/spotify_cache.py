"""Bounded metadata and artwork caches for Spotify-backed views."""

from __future__ import annotations

import hashlib
import json
import os
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path


class SpotifyCache:
    def __init__(self, root: str | Path, metadata_ttl_s: int = 3600):
        self.root = Path(root).expanduser()
        self.metadata = self.root / "metadata"
        self.artwork = self.root / "artwork"
        self.metadata_ttl_s = metadata_ttl_s
        self.metadata.mkdir(parents=True, exist_ok=True)
        self.artwork.mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _key(value: str) -> str:
        return hashlib.sha256(value.encode("utf-8")).hexdigest()

    def put_metadata(self, namespace: str, key: str, payload: object) -> Path:
        target = self.metadata / f"{namespace}-{self._key(key)}.json"
        self._atomic_write(target, json.dumps({"cached_at": int(time.time()), "payload": payload}).encode("utf-8"))
        return target

    def get_metadata(self, namespace: str, key: str, allow_stale: bool = False) -> object | None:
        target = self.metadata / f"{namespace}-{self._key(key)}.json"
        if not target.exists():
            return None
        try:
            wrapped = json.loads(target.read_text(encoding="utf-8"))
            age = int(time.time()) - int(wrapped["cached_at"])
            if age > self.metadata_ttl_s and not allow_stale:
                return None
            return wrapped["payload"]
        except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError):
            return None

    def cache_artwork(self, url: str) -> Path:
        suffix = Path(url.split("?", 1)[0]).suffix.lower()
        if suffix not in {".jpg", ".jpeg", ".png", ".webp"}:
            suffix = ".img"
        target = self.artwork / f"{self._key(url)}{suffix}"
        if target.exists() and target.stat().st_size:
            return target
        request = urllib.request.Request(url, headers={"User-Agent": "SHAeR/1.0"})
        try:
            with urllib.request.urlopen(request, timeout=20) as response:
                content_type = response.headers.get("Content-Type", "")
                if not content_type.startswith("image/"):
                    raise ValueError("Artwork URL did not return an image.")
                data = response.read(8 * 1024 * 1024 + 1)
        except (urllib.error.URLError, TimeoutError) as exc:
            raise RuntimeError("Spotify artwork is temporarily unreachable.") from exc
        if len(data) > 8 * 1024 * 1024:
            raise ValueError("Spotify artwork exceeded the cache size limit.")
        self._atomic_write(target, data)
        return target

    @staticmethod
    def _atomic_write(target: Path, data: bytes) -> None:
        target.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix=".shaer-cache-", dir=target.parent)
        try:
            with os.fdopen(fd, "wb") as handle:
                handle.write(data)
                handle.flush()
                os.fsync(handle.fileno())
            os.replace(temporary, target)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

