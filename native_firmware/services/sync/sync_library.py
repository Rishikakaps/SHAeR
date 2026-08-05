#!/usr/bin/env python3
"""Local music sync/index starter.

Scans a folder and creates a simple JSON library index for MP3, FLAC, and WAV.
ReplayGain fields are present from day one so the audio engine has stable
metadata names when tag extraction is added.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


SUPPORTED = {".mp3", ".flac", ".wav"}


def index_music(root: Path) -> list[dict]:
    tracks: list[dict] = []
    for path in sorted(root.rglob("*")):
        if path.suffix.lower() not in SUPPORTED:
            continue
        tracks.append(
            {
                "path": str(path),
                "title": path.stem,
                "codec": path.suffix.lower().lstrip(".").upper(),
                "replaygain_track_gain_db": None,
                "replaygain_album_gain_db": None,
                "replaygain_track_peak": None,
                "replaygain_album_peak": None,
            }
        )
    return tracks


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("music_dir", type=Path)
    parser.add_argument("--output", type=Path, default=Path("library_index.json"))
    args = parser.parse_args()

    tracks = index_music(args.music_dir)
    args.output.write_text(json.dumps({"tracks": tracks}, indent=2) + "\n")
    print(f"Indexed {len(tracks)} supported tracks into {args.output}")


if __name__ == "__main__":
    main()

