"""Source-neutral media provider contracts for local and remote libraries."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Iterable, Mapping, Protocol

from .library import TrackRepository


@dataclass(frozen=True, slots=True)
class MediaItem:
    provider: str
    media_id: str
    title: str
    artist: str = ""
    album: str = ""
    duration_ms: int = 0
    artwork: str | None = None
    playable: bool = True
    metadata: Mapping[str, object] = field(default_factory=dict)


@dataclass(frozen=True, slots=True)
class ProviderCapabilities:
    browse: bool = False
    search: bool = False
    playback: bool = False
    queue: bool = False
    metadata: bool = False


class LibraryProvider(Protocol):
    provider_id: str
    capabilities: ProviderCapabilities

    def browse(self, *, limit: int = 200) -> list[MediaItem]: ...
    def search(self, query: str, *, limit: int = 200) -> list[MediaItem]: ...


class PlaybackProvider(Protocol):
    provider_id: str

    def play(self, item: MediaItem) -> None: ...
    def pause(self) -> None: ...
    def next(self) -> None: ...
    def previous(self) -> None: ...


class ProviderRegistry:
    def __init__(self, providers: Iterable[LibraryProvider] = ()):
        self._providers: dict[str, LibraryProvider] = {}
        for provider in providers:
            self.register(provider)

    def register(self, provider: LibraryProvider) -> None:
        if provider.provider_id in self._providers:
            raise ValueError(f"Provider already registered: {provider.provider_id}")
        self._providers[provider.provider_id] = provider

    def get(self, provider_id: str) -> LibraryProvider:
        try:
            return self._providers[provider_id]
        except KeyError as exc:
            raise LookupError(f"Unknown media provider: {provider_id}") from exc

    def browse(self, provider_id: str, *, limit: int = 200) -> list[MediaItem]:
        provider = self.get(provider_id)
        if not provider.capabilities.browse:
            raise NotImplementedError(f"{provider_id} does not support browsing")
        return provider.browse(limit=limit)

    def search(self, query: str, *, limit: int = 200) -> list[MediaItem]:
        results: list[MediaItem] = []
        for provider in self._providers.values():
            if provider.capabilities.search:
                results.extend(provider.search(query, limit=max(0, limit - len(results))))
            if len(results) >= limit:
                break
        return results[:limit]

    def capabilities(self) -> dict[str, ProviderCapabilities]:
        return {provider_id: provider.capabilities for provider_id, provider in self._providers.items()}


class LocalLibraryProvider:
    provider_id = "local"
    capabilities = ProviderCapabilities(browse=True, search=True, playback=True, queue=True, metadata=True)

    def __init__(self, tracks: TrackRepository):
        self.tracks = tracks

    @staticmethod
    def _media(row: Mapping[str, object]) -> MediaItem:
        track_id = str(row["id"])
        return MediaItem(
            provider="local",
            media_id=track_id,
            title=str(row.get("title") or "Unknown track"),
            artist=str(row.get("artist") or ""),
            album=str(row.get("album") or ""),
            duration_ms=max(0, int(row.get("duration_s") or 0) * 1000),
            artwork=str(row.get("cover_art_path")) if row.get("cover_art_path") else None,
            playable=bool(row.get("filepath")),
            metadata={"filepath": str(row.get("filepath") or "")},
        )

    def browse(self, *, limit: int = 200) -> list[MediaItem]:
        return [self._media(dict(row)) for row in self.tracks.all()[: max(0, limit)]]

    def search(self, query: str, *, limit: int = 200) -> list[MediaItem]:
        return [self._media(dict(row)) for row in self.tracks.search(query)[: max(0, limit)]]
