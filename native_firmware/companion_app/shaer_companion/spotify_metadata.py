from __future__ import annotations

import json
from urllib.parse import quote
from urllib.request import Request, urlopen

from .database import DatabaseManager
from .events import Event, EventBus


class SpotifyMetadataSync:
    def __init__(self, db: DatabaseManager, events: EventBus, access_token: str = "") -> None:
        self.db = db
        self.events = events
        self.access_token = access_token

    def set_access_token(self, token: str) -> None:
        self.access_token = token
        self.events.publish(Event("spotify.token.updated", "SpotifyMetadataSync"))

    def search_track(self, title: str, artist: str) -> dict:
        if not self.access_token:
            raise RuntimeError("Spotify access token is not configured")
        query = quote(f"track:{title} artist:{artist}")
        request = Request(
            f"https://api.spotify.com/v1/search?q={query}&type=track&limit=1",
            headers={"Authorization": f"Bearer {self.access_token}"},
        )
        with urlopen(request, timeout=20) as response:
            return json.loads(response.read().decode("utf-8"))

    def enrich_local_track(self, content_hash: str, spotify_uri: str, album_art_url: str, popularity: int | None) -> None:
        with self.db.connect(self.db.library_db) as conn:
            conn.execute(
                "UPDATE tracks SET spotify_uri = ?, album_art_url = ?, popularity = ? WHERE content_hash = ?",
                (spotify_uri, album_art_url, popularity, content_hash),
            )
        self.events.publish(Event("spotify.metadata.enriched", "SpotifyMetadataSync", {"track_hash": content_hash}))

