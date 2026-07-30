#!/usr/bin/env python3
import tempfile

from shaer_music import SpotifyCache


def main():
    with tempfile.TemporaryDirectory() as tmp:
        cache = SpotifyCache(tmp)
        cache.put_metadata("album", "spotify:album:1", {"name": "Album"})
        assert cache.get_metadata("album", "spotify:album:1") == {"name": "Album"}
    print("spotify_cache_test ok metadata_roundtrip=true")


if __name__ == "__main__":
    main()
