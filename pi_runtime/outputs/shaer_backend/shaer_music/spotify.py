"""Spotify metadata helpers that keep SHAeR's UI source-agnostic."""

from __future__ import annotations

import difflib
import re
from dataclasses import dataclass
from typing import Mapping


@dataclass(slots=True)
class SpotifyTrack:
    spotify_track_id: str
    title: str
    artist: str
    album: str | None = None
    duration_ms: int | None = None
    isrc: str | None = None


def normalize(value: str | None) -> str:
    if not value:
        return ""
    return re.sub(r"[^a-z0-9]+", " ", value.lower()).strip()


class LocalLibraryMatcher:
    """Implements Doc 15's local/Spotify matching confidence rules."""

    def confidence(self, spotify_track: SpotifyTrack, local_track: Mapping[str, object]) -> int:
        local_isrc = str(local_track.get("isrc") or "")
        if spotify_track.isrc and local_isrc and spotify_track.isrc == local_isrc:
            return 100

        title_match = normalize(spotify_track.title) == normalize(str(local_track.get("title") or ""))
        artist_match = normalize(spotify_track.artist) == normalize(str(local_track.get("artist") or ""))
        album_match = normalize(spotify_track.album) == normalize(str(local_track.get("album") or ""))
        if title_match and artist_match and album_match:
            return 90

        local_duration_s = local_track.get("duration_s")
        local_duration_ms = int(local_duration_s) * 1000 if local_duration_s is not None else None
        duration_close = (
            spotify_track.duration_ms is not None
            and local_duration_ms is not None
            and abs(spotify_track.duration_ms - local_duration_ms) < 3000
        )
        fuzzy_title = difflib.SequenceMatcher(
            None, normalize(spotify_track.title), normalize(str(local_track.get("title") or ""))
        ).ratio()
        if fuzzy_title > 0.85 and artist_match and duration_close:
            return 80
        return 0

    def is_auto_match(self, spotify_track: SpotifyTrack, local_track: Mapping[str, object]) -> bool:
        return self.confidence(spotify_track, local_track) >= 80
