"""Source-neutral playback state shared by local and Spotify playback."""

from __future__ import annotations

from dataclasses import asdict, dataclass
from typing import Mapping


@dataclass(slots=True)
class PlaybackState:
    title: str = ""
    artist: str = ""
    album: str = ""
    duration_ms: int = 0
    progress_ms: int = 0
    cover_art: str | None = None
    status: str = "stopped"
    queue_position: int | None = None
    volume_percent: int | None = None
    source: str = "local"
    uri: str | None = None

    def public_dict(self) -> dict[str, object]:
        return asdict(self)


def spotify_playback_state(payload: Mapping[str, object] | None) -> PlaybackState:
    if not payload:
        return PlaybackState(source="spotify")
    item = payload.get("item")
    track = item if isinstance(item, dict) else {}
    album_value = track.get("album")
    album = album_value if isinstance(album_value, dict) else {}
    artists_value = track.get("artists")
    artists = artists_value if isinstance(artists_value, list) else []
    images_value = album.get("images")
    images = images_value if isinstance(images_value, list) else []
    first_image = images[0] if images and isinstance(images[0], dict) else {}
    device_value = payload.get("device")
    device = device_value if isinstance(device_value, dict) else {}
    artist_names = [str(artist.get("name")) for artist in artists if isinstance(artist, dict) and artist.get("name")]
    return PlaybackState(
        title=str(track.get("name") or ""),
        artist=", ".join(artist_names),
        album=str(album.get("name") or ""),
        duration_ms=int(track.get("duration_ms") or 0),
        progress_ms=int(payload.get("progress_ms") or 0),
        cover_art=str(first_image.get("url")) if first_image.get("url") else None,
        status="playing" if payload.get("is_playing") else "paused",
        volume_percent=int(device.get("volume_percent")) if device.get("volume_percent") is not None else None,
        source="spotify",
        uri=str(track.get("uri")) if track.get("uri") else None,
    )
