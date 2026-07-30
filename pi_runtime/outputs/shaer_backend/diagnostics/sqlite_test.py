#!/usr/bin/env python3
from pathlib import Path
import tempfile

from shaer_music import LibraryDatabase


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        db = LibraryDatabase(Path(tmp) / "library.db")
        db.migrate()
        version = db.connection.execute("SELECT max(version) AS version FROM schema_migrations").fetchone()["version"]
        journal = db.connection.execute("PRAGMA journal_mode").fetchone()[0]
        synchronous = db.connection.execute("PRAGMA synchronous").fetchone()[0]
        timeout = db.connection.execute("PRAGMA busy_timeout").fetchone()[0]
        assert str(journal).lower() == "wal"
        assert int(synchronous) == 2
        assert int(timeout) >= 5000
        db.close()
    print(f"sqlite_test ok schema_version={version} wal=true synchronous=full")


if __name__ == "__main__":
    main()
