#!/usr/bin/env python3
from shaer_music import SpotifyClient


class Auth:
    def access_token(self, force_refresh=False):
        return "redacted"


def main():
    client = SpotifyClient(Auth(), transport=lambda *_args: (200, {}, b'{"id":"shaer-user"}'))
    assert client.current_user() == {"id": "shaer-user"}
    print("spotify_api_test ok centralized_client=true")


if __name__ == "__main__":
    main()
