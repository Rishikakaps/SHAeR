from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


SUPPORTED_AUDIO_EXTENSIONS = {".mp3", ".flac", ".wav", ".aac", ".m4a"}


@dataclass(frozen=True)
class TrackMetadata:
    title: str
    artist: str
    album: str
    genre: str = ""
    year: str = ""
    track_number: str = ""
    duration_seconds: float = 0.0
    source_path: Path = Path()
    file_format: str = ""
    file_size: int = 0
    content_hash: str = ""
    artwork_path: Path | None = None
    spotify_uri: str = ""
    album_art_url: str = ""
    popularity: int | None = None
    provider: str = "local_folder"


@dataclass(frozen=True)
class PlaylistMapping:
    name: str
    track_hashes: list[str] = field(default_factory=list)
    artwork_path: Path | None = None


@dataclass(frozen=True)
class ImportResult:
    tracks: list[TrackMetadata]
    playlists: list[PlaylistMapping] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)


@dataclass(frozen=True)
class SyncPlanItem:
    track_hash: str
    source_path: Path
    destination_path: Path
    action: str
    bytes_to_copy: int


@dataclass(frozen=True)
class SyncPlan:
    items: list[SyncPlanItem]
    skipped_duplicates: int = 0

    @property
    def total_bytes(self) -> int:
        return sum(item.bytes_to_copy for item in self.items)

