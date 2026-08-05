from __future__ import annotations

import hashlib
import struct
import wave
from pathlib import Path

from ..models import ImportResult, SUPPORTED_AUDIO_EXTENSIONS, TrackMetadata
from .base import ImportProvider


def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def clean_text(value: bytes) -> str:
    return value.rstrip(b"\x00 ").decode("latin1", errors="replace").strip()


def syncsafe(value: bytes) -> int:
    return (value[0] << 21) | (value[1] << 14) | (value[2] << 7) | value[3]


def decode_id3_text(data: bytes) -> str:
    if not data:
        return ""
    encoding = data[0]
    payload = data[1:]
    if encoding == 1:
        return payload.decode("utf-16", errors="replace").strip("\x00").strip()
    if encoding == 2:
        return payload.decode("utf-16-be", errors="replace").strip("\x00").strip()
    if encoding == 3:
        return payload.decode("utf-8", errors="replace").strip("\x00").strip()
    return payload.decode("latin1", errors="replace").strip("\x00").strip()


def parse_mp3(path: Path) -> dict[str, str | float]:
    metadata: dict[str, str | float] = {}
    data = path.read_bytes()[:1024 * 512]
    if data.startswith(b"ID3") and len(data) >= 10:
        size = syncsafe(data[6:10])
        offset = 10
        end = min(len(data), 10 + size)
        frames = {"TIT2": "title", "TPE1": "artist", "TALB": "album", "TCON": "genre", "TDRC": "year", "TYER": "year", "TRCK": "track_number"}
        while offset + 10 <= end:
            frame_id = data[offset:offset + 4].decode("latin1", errors="ignore")
            frame_size = int.from_bytes(data[offset + 4:offset + 8], "big")
            if frame_size <= 0:
                break
            payload = data[offset + 10:offset + 10 + frame_size]
            if frame_id in frames:
                metadata[frames[frame_id]] = decode_id3_text(payload)
            offset += 10 + frame_size
    if not metadata:
        with path.open("rb") as f:
            f.seek(max(0, path.stat().st_size - 128))
            tag = f.read(128)
        if tag.startswith(b"TAG"):
            metadata["title"] = clean_text(tag[3:33])
            metadata["artist"] = clean_text(tag[33:63])
            metadata["album"] = clean_text(tag[63:93])
            metadata["year"] = clean_text(tag[93:97])
            metadata["genre"] = str(tag[127]) if len(tag) == 128 else ""
    return metadata


def parse_wav(path: Path) -> dict[str, str | float]:
    metadata: dict[str, str | float] = {}
    try:
        with wave.open(str(path), "rb") as wav:
            frames = wav.getnframes()
            rate = wav.getframerate()
            metadata["duration_seconds"] = frames / float(rate) if rate else 0.0
    except (wave.Error, EOFError):
        metadata["duration_seconds"] = 0.0
    return metadata


def parse_flac(path: Path) -> dict[str, str | float]:
    metadata: dict[str, str | float] = {}
    with path.open("rb") as f:
        if f.read(4) != b"fLaC":
            return metadata
        last = False
        while not last:
            header = f.read(4)
            if len(header) < 4:
                break
            block_type = header[0] & 0x7F
            last = bool(header[0] & 0x80)
            length = int.from_bytes(header[1:4], "big")
            block = f.read(length)
            if block_type == 4 and len(block) >= 8:
                vendor_len = struct.unpack_from("<I", block, 0)[0]
                offset = 4 + vendor_len
                if offset + 4 > len(block):
                    break
                count = struct.unpack_from("<I", block, offset)[0]
                offset += 4
                for _ in range(count):
                    if offset + 4 > len(block):
                        break
                    comment_len = struct.unpack_from("<I", block, offset)[0]
                    offset += 4
                    comment = block[offset:offset + comment_len].decode("utf-8", errors="replace")
                    offset += comment_len
                    if "=" in comment:
                        key, value = comment.split("=", 1)
                        key_map = {
                            "TITLE": "title",
                            "ARTIST": "artist",
                            "ALBUM": "album",
                            "GENRE": "genre",
                            "DATE": "year",
                            "TRACKNUMBER": "track_number",
                        }
                        if key.upper() in key_map:
                            metadata[key_map[key.upper()]] = value
                break
    return metadata


def parse_basic_atoms(path: Path) -> dict[str, str | float]:
    metadata: dict[str, str | float] = {}
    with path.open("rb") as f:
        header = f.read(12)
        if len(header) >= 8 and header[4:8] in {b"ftyp", b"moov", b"mdat"}:
            metadata["container"] = "mp4"
    return metadata


class LocalFolderProvider(ImportProvider):
    provider_id = "local_folder"
    display_name = "Local Folder"

    def scan(self, source: Path) -> ImportResult:
        tracks: list[TrackMetadata] = []
        errors: list[str] = []
        for path in sorted(source.rglob("*")):
            if not path.is_file() or path.suffix.lower() not in SUPPORTED_AUDIO_EXTENSIONS:
                continue
            try:
                metadata = self._metadata_for(path)
                title = str(metadata.get("title") or path.stem)
                artist = str(metadata.get("artist") or "Unknown Artist")
                album = str(metadata.get("album") or "Unknown Album")
                tracks.append(
                    TrackMetadata(
                        title=title,
                        artist=artist,
                        album=album,
                        genre=str(metadata.get("genre") or ""),
                        year=str(metadata.get("year") or ""),
                        track_number=str(metadata.get("track_number") or ""),
                        duration_seconds=float(metadata.get("duration_seconds") or 0.0),
                        source_path=path,
                        file_format=path.suffix.lower().removeprefix("."),
                        file_size=path.stat().st_size,
                        content_hash=file_hash(path),
                        provider=self.provider_id,
                    )
                )
            except OSError as exc:
                errors.append(f"{path}: {exc}")
        return ImportResult(tracks=tracks, errors=errors)

    def _metadata_for(self, path: Path) -> dict[str, str | float]:
        suffix = path.suffix.lower()
        if suffix == ".mp3":
            return parse_mp3(path)
        if suffix == ".wav":
            return parse_wav(path)
        if suffix == ".flac":
            return parse_flac(path)
        if suffix in {".m4a", ".aac"}:
            return parse_basic_atoms(path)
        return {}

