#!/usr/bin/env python3
from __future__ import annotations

import tarfile
import os
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
RELEASES = ROOT / "releases"
ARCHIVE = RELEASES / f"shaer-{VERSION}.tar.gz"

EXCLUDED_DIRS = {
    ".git",
    ".companion_data",
    "build",
    "build_cmake",
    "releases",
    "work",
    "__pycache__",
    ".pytest_cache",
    ".venv",
    "venv",
}

EXCLUDED_SUFFIXES = {
    ".pyc",
    ".pyo",
    ".db",
    ".sqlite",
    ".sqlite3",
    ".zip",
    ".tar.gz",
}


def should_exclude(path: Path) -> bool:
    relative = path.relative_to(ROOT)
    parts = set(relative.parts)
    if parts & EXCLUDED_DIRS:
        return True
    name = path.name
    if name == ".DS_Store":
        return True
    return any(name.endswith(suffix) for suffix in EXCLUDED_SUFFIXES)


def main() -> int:
    RELEASES.mkdir(exist_ok=True)
    if ARCHIVE.exists():
        ARCHIVE.unlink()

    with tarfile.open(ARCHIVE, "w:gz") as archive:
        for current, dirs, files in os.walk(ROOT):
            current_path = Path(current)
            dirs[:] = sorted(
                directory
                for directory in dirs
                if not should_exclude(current_path / directory)
            )
            for filename in sorted(files):
                path = current_path / filename
                if should_exclude(path):
                    continue
                archive.add(
                    path,
                    arcname=Path(f"shaer-{VERSION}") / path.relative_to(ROOT),
                    recursive=False,
                )

    print(ARCHIVE)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
