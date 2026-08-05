"""Spotify OAuth 2.0 PKCE authentication for SHAeR.

The module intentionally uses only the Python standard library.  Tokens are
never returned by status methods or written to logs.
"""

from __future__ import annotations

import base64
import hashlib
import json
import os
import secrets
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
import webbrowser
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Mapping


AUTHORIZE_URL = "https://accounts.spotify.com/authorize"
TOKEN_URL = "https://accounts.spotify.com/api/token"
DEFAULT_SCOPES = (
    "user-read-private",
    "user-library-read",
    "user-follow-read",
    "playlist-read-private",
    "playlist-read-collaborative",
    "user-read-recently-played",
    "user-read-playback-state",
    "user-read-currently-playing",
    "user-modify-playback-state",
)


class SpotifyAuthError(RuntimeError):
    """A safe, token-free authentication error."""


class LoginCancelled(SpotifyAuthError):
    pass


class LoginExpired(SpotifyAuthError):
    pass


@dataclass(frozen=True, slots=True)
class SpotifyToken:
    access_token: str
    refresh_token: str | None
    expires_at: int
    scope: str
    token_type: str = "Bearer"

    @classmethod
    def from_response(
        cls,
        payload: Mapping[str, object],
        previous_refresh_token: str | None = None,
        now: int | None = None,
    ) -> "SpotifyToken":
        access_token = str(payload.get("access_token") or "")
        if not access_token:
            raise SpotifyAuthError("Spotify did not return an access token.")
        expires_in = max(1, int(payload.get("expires_in") or 3600))
        return cls(
            access_token=access_token,
            refresh_token=str(payload.get("refresh_token") or previous_refresh_token or "") or None,
            expires_at=int(now if now is not None else time.time()) + expires_in,
            scope=str(payload.get("scope") or ""),
            token_type=str(payload.get("token_type") or "Bearer"),
        )

    def needs_refresh(self, leeway_s: int = 90, now: int | None = None) -> bool:
        current = int(now if now is not None else time.time())
        return current + leeway_s >= self.expires_at


class TokenStore:
    """Atomic JSON credential store with owner-only permissions."""

    def __init__(self, path: str | Path):
        self.path = Path(path).expanduser()
        self._lock = threading.RLock()

    def save(self, token: SpotifyToken) -> None:
        payload = {
            "access_token": token.access_token,
            "refresh_token": token.refresh_token,
            "expires_at": token.expires_at,
            "scope": token.scope,
            "token_type": token.token_type,
        }
        with self._lock:
            self.path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
            try:
                self.path.parent.chmod(0o700)
            except OSError:
                pass
            fd, temporary = tempfile.mkstemp(prefix=".spotify-token-", dir=self.path.parent)
            try:
                os.fchmod(fd, 0o600)
                with os.fdopen(fd, "w", encoding="utf-8") as handle:
                    json.dump(payload, handle, separators=(",", ":"))
                    handle.flush()
                    os.fsync(handle.fileno())
                os.replace(temporary, self.path)
                self.path.chmod(0o600)
            finally:
                if os.path.exists(temporary):
                    os.unlink(temporary)

    def load(self) -> SpotifyToken | None:
        with self._lock:
            if not self.path.exists():
                return None
            try:
                payload = json.loads(self.path.read_text(encoding="utf-8"))
                return SpotifyToken(
                    access_token=str(payload["access_token"]),
                    refresh_token=str(payload.get("refresh_token") or "") or None,
                    expires_at=int(payload["expires_at"]),
                    scope=str(payload.get("scope") or ""),
                    token_type=str(payload.get("token_type") or "Bearer"),
                )
            except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
                raise SpotifyAuthError("Stored Spotify credentials are unreadable; log in again.") from exc

    def clear(self) -> None:
        with self._lock:
            try:
                self.path.unlink()
            except FileNotFoundError:
                pass


@dataclass(slots=True)
class LoginAttempt:
    state: str
    verifier: str
    authorization_url: str
    created_at: float
    timeout_s: int
    cancelled: bool = False

    @property
    def expires_at(self) -> float:
        return self.created_at + self.timeout_s


def _base64url(value: bytes) -> str:
    return base64.urlsafe_b64encode(value).decode("ascii").rstrip("=")


def create_pkce_pair() -> tuple[str, str]:
    verifier = _base64url(secrets.token_bytes(64))
    challenge = _base64url(hashlib.sha256(verifier.encode("ascii")).digest())
    return verifier, challenge


HttpPost = Callable[[str, Mapping[str, str]], Mapping[str, object]]


def _token_post(url: str, form: Mapping[str, str]) -> Mapping[str, object]:
    request = urllib.request.Request(
        url,
        data=urllib.parse.urlencode(form).encode("utf-8"),
        headers={"Content-Type": "application/x-www-form-urlencoded", "Accept": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        raise SpotifyAuthError(f"Spotify authentication failed with HTTP {exc.code}.") from exc
    except (urllib.error.URLError, TimeoutError) as exc:
        raise SpotifyAuthError("Spotify authentication is temporarily unreachable.") from exc


class SpotifyAuthManager:
    """Owns PKCE attempts and persistent Spotify credentials."""

    def __init__(
        self,
        client_id: str,
        redirect_uri: str,
        token_store: TokenStore,
        scopes: tuple[str, ...] = DEFAULT_SCOPES,
        token_post: HttpPost = _token_post,
    ):
        if not client_id:
            raise ValueError("SPOTIFY_CLIENT_ID is required.")
        if not redirect_uri:
            raise ValueError("SPOTIFY_REDIRECT_URI is required.")
        self.client_id = client_id
        self.redirect_uri = redirect_uri
        self.token_store = token_store
        self.scopes = scopes
        self._token_post = token_post
        self._attempts: dict[str, LoginAttempt] = {}
        self._lock = threading.RLock()

    def begin_login(self, timeout_s: int = 300, launch_browser: bool = False) -> LoginAttempt:
        verifier, challenge = create_pkce_pair()
        state = secrets.token_urlsafe(32)
        query = urllib.parse.urlencode(
            {
                "client_id": self.client_id,
                "response_type": "code",
                "redirect_uri": self.redirect_uri,
                "scope": " ".join(self.scopes),
                "state": state,
                "code_challenge_method": "S256",
                "code_challenge": challenge,
            }
        )
        attempt = LoginAttempt(state, verifier, f"{AUTHORIZE_URL}?{query}", time.time(), timeout_s)
        with self._lock:
            self._expire_attempts()
            self._attempts[state] = attempt
        if launch_browser:
            webbrowser.open(attempt.authorization_url, new=1, autoraise=True)
        return attempt

    def cancel_login(self, state: str | None = None) -> bool:
        with self._lock:
            if state:
                attempt = self._attempts.get(state)
                if not attempt:
                    return False
                attempt.cancelled = True
                return True
            changed = bool(self._attempts)
            for attempt in self._attempts.values():
                attempt.cancelled = True
            return changed

    def complete_login(self, state: str, code: str | None, error: str | None = None) -> SpotifyToken:
        with self._lock:
            attempt = self._attempts.pop(state, None)
        if attempt is None:
            raise SpotifyAuthError("The Spotify login state is invalid or has already been used.")
        if attempt.cancelled:
            raise LoginCancelled("Spotify login was cancelled.")
        if time.time() >= attempt.expires_at:
            raise LoginExpired("Spotify login timed out.")
        if error:
            raise SpotifyAuthError(f"Spotify login was not approved ({error}).")
        if not code:
            raise SpotifyAuthError("Spotify callback did not include an authorization code.")
        payload = self._token_post(
            TOKEN_URL,
            {
                "client_id": self.client_id,
                "grant_type": "authorization_code",
                "code": code,
                "redirect_uri": self.redirect_uri,
                "code_verifier": attempt.verifier,
            },
        )
        token = SpotifyToken.from_response(payload)
        self.token_store.save(token)
        return token

    def access_token(self, force_refresh: bool = False) -> str:
        token = self.token_store.load()
        if token is None:
            raise SpotifyAuthError("Spotify is not logged in.")
        if force_refresh or token.needs_refresh():
            token = self.refresh(token)
        return token.access_token

    def refresh(self, token: SpotifyToken | None = None) -> SpotifyToken:
        with self._lock:
            current = token or self.token_store.load()
            if current is None or not current.refresh_token:
                self.token_store.clear()
                raise SpotifyAuthError("Spotify needs to be authorized again.")
            payload = self._token_post(
                TOKEN_URL,
                {
                    "client_id": self.client_id,
                    "grant_type": "refresh_token",
                    "refresh_token": current.refresh_token,
                },
            )
            refreshed = SpotifyToken.from_response(payload, current.refresh_token)
            self.token_store.save(refreshed)
            return refreshed

    def logout(self) -> None:
        self.cancel_login()
        self.token_store.clear()

    def status(self) -> dict[str, object]:
        token = self.token_store.load()
        with self._lock:
            self._expire_attempts()
            pending = any(not attempt.cancelled for attempt in self._attempts.values())
        return {
            "configured": True,
            "authenticated": token is not None and bool(token.refresh_token or not token.needs_refresh()),
            "expires_at": token.expires_at if token else None,
            "login_pending": pending,
        }

    def _expire_attempts(self) -> None:
        now = time.time()
        expired = [state for state, attempt in self._attempts.items() if now >= attempt.expires_at]
        for state in expired:
            self._attempts.pop(state, None)

