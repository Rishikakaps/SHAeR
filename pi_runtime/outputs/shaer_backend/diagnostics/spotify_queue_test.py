#!/usr/bin/env python3
from shaer_music import SpotifyClient


class Auth:
    def access_token(self, force_refresh=False):
        return "redacted"


def main():
    expected = {"currently_playing": {"uri": "spotify:track:1"}, "queue": [{"uri": "spotify:track:2"}]}
    client = SpotifyClient(Auth(), transport=lambda *_args: (200, {}, b'{"currently_playing":{"uri":"spotify:track:1"},"queue":[{"uri":"spotify:track:2"}]}'))
    assert client.queue() == expected
    print("spotify_queue_test ok queue_items=1")


if __name__ == "__main__":
    main()
