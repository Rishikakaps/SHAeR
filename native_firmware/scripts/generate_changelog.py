#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys


def main() -> int:
    try:
        raw = subprocess.check_output(
            ["git", "log", "--pretty=format:%s", "--no-merges"],
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        print(f"Could not read Git history: {exc}", file=sys.stderr)
        return 1

    groups = {
        "feat": [],
        "fix": [],
        "docs": [],
        "test": [],
        "chore": [],
    }

    for line in raw.splitlines():
        kind = line.split(":", 1)[0].split("(", 1)[0]
        groups.setdefault(kind, []).append(line)

    for title, kind in [
        ("Features", "feat"),
        ("Fixes", "fix"),
        ("Docs", "docs"),
        ("Tests", "test"),
        ("Maintenance", "chore"),
    ]:
        if groups.get(kind):
            print(f"## {title}")
            for entry in groups[kind]:
                print(f"- {entry}")
            print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
