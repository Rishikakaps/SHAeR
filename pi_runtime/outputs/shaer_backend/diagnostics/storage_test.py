#!/usr/bin/env python3
import tempfile
from pathlib import Path

from shaer_music import LibraryDatabase
from shaer_recording import RecordingArchive


def main():
    with tempfile.TemporaryDirectory() as tmp:
        db = LibraryDatabase(Path(tmp) / "library.db")
        db.migrate()
        assert db.connection.execute("PRAGMA integrity_check").fetchone()[0] == "ok"
        db.close()
        recordings = RecordingArchive(Path(tmp) / "recordings.db", Path(tmp) / "Recordings")
        assert recordings.connection.execute("PRAGMA integrity_check").fetchone()[0] == "ok"
        usage = recordings.storage()
        assert usage["free"] > 0 and usage["recordings"] == 0
        recordings.close()
    print("storage_test ok sqlite_integrity=true recording_archive=true")


if __name__ == "__main__": main()
