#!/usr/bin/env python3
from pathlib import Path
import tempfile

from shaer_music import LibraryDatabase, LibraryIndexer


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        music = root / "music"
        music.mkdir()
        (music / "local-demo.mp3").write_bytes(b"demo")
        db = LibraryDatabase(root / "library.db")
        db.migrate()
        report = LibraryIndexer(db, music).index()
        db.close()
    print(f"library_test ok {report}")


if __name__ == "__main__":
    main()
