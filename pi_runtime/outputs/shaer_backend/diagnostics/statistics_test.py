#!/usr/bin/env python3
from pathlib import Path
import tempfile

from shaer_music import LibraryDatabase, SessionRepository, TrackRecord, TrackRepository


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        db = LibraryDatabase(Path(tmp) / "library.db")
        db.migrate()
        tracks = TrackRepository(db)
        track_id = tracks.upsert(TrackRecord(filepath=str(Path(tmp) / "song.mp3"), title="Song"))
        SessionRepository(db).record_listen(track_id, 31, "local")
        row = tracks.get(track_id)
        db.close()
    print(f"statistics_test ok play_count={row['play_count']} total={row['total_listened_s']}")


if __name__ == "__main__":
    main()
