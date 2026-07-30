"""SHAeR Layer 11 music backend primitives."""

from .library import (
    LibraryDatabase,
    LibraryIndexer,
    PlaybackQueue,
    PlaylistRepository,
    QueueRepository,
    RepeatMode,
    SessionRepository,
    StatisticsDatabase,
    TrackDatabase,
    TrackRecord,
    TrackRepository,
)
from .spotify import LocalLibraryMatcher, SpotifyTrack
from .spotify_auth import (
    DEFAULT_SCOPES,
    LoginAttempt,
    LoginCancelled,
    LoginExpired,
    SpotifyAuthError,
    SpotifyAuthManager,
    SpotifyToken,
    TokenStore,
    create_pkce_pair,
)
from .spotify_cache import SpotifyCache
from .spotify_client import SpotifyApiError, SpotifyClient
from .spotify_connect import ConnectStatus, LibrespotManager
from .spotify_playback import PlaybackState, spotify_playback_state
from .spotify_services import SpotifyServices
from .providers import (
    LibraryProvider,
    LocalLibraryProvider,
    MediaItem,
    PlaybackProvider,
    ProviderCapabilities,
    ProviderRegistry,
)

__all__ = [
    "LibraryDatabase",
    "LibraryIndexer",
    "LocalLibraryMatcher",
    "PlaybackQueue",
    "PlaylistRepository",
    "QueueRepository",
    "RepeatMode",
    "SessionRepository",
    "SpotifyTrack",
    "DEFAULT_SCOPES",
    "LoginAttempt",
    "LoginCancelled",
    "LoginExpired",
    "SpotifyAuthError",
    "SpotifyAuthManager",
    "SpotifyToken",
    "TokenStore",
    "create_pkce_pair",
    "SpotifyCache",
    "SpotifyApiError",
    "SpotifyClient",
    "ConnectStatus",
    "LibrespotManager",
    "PlaybackState",
    "spotify_playback_state",
    "SpotifyServices",
    "LibraryProvider",
    "LocalLibraryProvider",
    "MediaItem",
    "PlaybackProvider",
    "ProviderCapabilities",
    "ProviderRegistry",
    "StatisticsDatabase",
    "TrackDatabase",
    "TrackRecord",
    "TrackRepository",
]
