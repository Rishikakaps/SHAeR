#!/usr/bin/env python3
from shaer_music import LocalLibraryMatcher, SpotifyTrack


def main() -> None:
    matcher = LocalLibraryMatcher()
    score = matcher.confidence(
        SpotifyTrack("spotify:track:1", "Song", "Artist", "Album", 120000),
        {"title": "Song", "artist": "Artist", "album": "Album", "duration_s": 120},
    )
    print(f"spotify_matching_test ok confidence={score}")


if __name__ == "__main__":
    main()
