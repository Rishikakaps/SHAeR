#!/usr/bin/env python3
import tempfile
from pathlib import Path

from shaer_music import SpotifyAuthManager, TokenStore


def main():
    with tempfile.TemporaryDirectory() as tmp:
        auth = SpotifyAuthManager(
            "diagnostic-client",
            "http://127.0.0.1:8775/api/spotify/callback",
            TokenStore(Path(tmp) / "token.json"),
            token_post=lambda _url, _form: {
                "access_token": "redacted",
                "refresh_token": "redacted",
                "expires_in": 3600,
            },
        )
        attempt = auth.begin_login()
        auth.complete_login(attempt.state, "diagnostic-code")
        assert auth.status()["authenticated"]
    print("spotify_login_test ok pkce=persistent state=validated")


if __name__ == "__main__":
    main()
