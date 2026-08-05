#!/usr/bin/env python3
"""Tolerance-based PNG comparison for SHAeR theme baselines."""

from __future__ import annotations

import json
import sys
from pathlib import Path

from PIL import Image, ImageChops


def compare(expected: Path, actual: Path, threshold: int = 20) -> dict[str, object]:
    with Image.open(expected).convert("RGB") as left, Image.open(actual).convert("RGB") as right:
        if left.size != right.size:
            return {"match": False, "difference_ratio": 1.0, "reason": "dimension mismatch"}
        difference = ImageChops.difference(left, right)
        pixels = difference.getdata()
        changed = sum(1 for pixel in pixels if max(pixel) > threshold)
        total = max(1, left.width * left.height)
        ratio = changed / total
        return {"match": ratio <= 0.02, "difference_ratio": round(ratio, 6), "reason": "pixel tolerance"}


def main() -> None:
    baseline_root = Path(sys.argv[1])
    artifact_root = Path(sys.argv[2])
    result: dict[str, object] = {"files": {}, "mismatches": 0}
    for baseline in sorted(baseline_root.rglob("*.png")):
        relative = baseline.relative_to(baseline_root)
        actual = artifact_root / relative
        if not actual.exists():
            comparison = {"match": False, "difference_ratio": 1.0, "reason": "artifact missing"}
        else:
            comparison = compare(baseline, actual)
        result["files"][str(relative)] = comparison
        if not comparison["match"]:
            result["mismatches"] += 1
    print(json.dumps(result))


if __name__ == "__main__":
    main()
