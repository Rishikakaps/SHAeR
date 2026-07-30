#!/usr/bin/env python3
from shaer_music import spotify_playback_state


def main():
    state = spotify_playback_state({
        "is_playing": True,
        "progress_ms": 1000,
        "item": {
            "name": "Song",
            "duration_ms": 2000,
            "artists": [{"name": "Artist"}],
            "album": {"name": "Album", "images": []},
        },
    })
    assert state.title == "Song" and state.artist == "Artist" and state.source == "spotify"
    print("spotify_metadata_test ok source_neutral=true")


if __name__ == "__main__":
    main()
