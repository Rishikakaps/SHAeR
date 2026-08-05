#!/usr/bin/env python3
"""Checks that the static visual simulator preserves six distinct theme worlds."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / "simulator" / "web" / "index.html"


def main() -> None:
    text = HTML.read_text()

    theme_ids = re.findall(r'id:"([^"]+)"', text)
    assert theme_ids == ["archive", "bombay", "raga", "xp", "punk", "ghibli"], theme_ids

    for screen in ["boot", "home", "library", "now", "settings", "spotify", "wifi", "bluetooth", "charging", "sleep"]:
        assert f'data-screen="{screen}"' in text, f"{screen} screen control missing"

    css_theme_classes = re.findall(r"\n\s*\.(archive|bombay|raga|xp|punk|ghibli)\s*\{", text)
    assert sorted(set(css_theme_classes)) == ["archive", "bombay", "ghibli", "punk", "raga", "xp"]

    assert "battery-saver" in text
    assert "Spotify slipped" in text
    assert "WiFi wandered off" in text
    assert "Headphones disconnected" in text
    assert "LIBRA CONSTELLATION BOOT" in text
    assert "Loading D: Drive... mhm mhm" in text
    assert "CHARGING" in text
    assert "toLocaleTimeString" in text
    assert "hour12:true" in text

    raga_block = re.search(r"\.raga\s*\{(?P<body>.*?)\.xp\s*\{", text, flags=re.S)
    assert raga_block, "Indian Raga CSS block missing"
    raga_css = raga_block.group("body")
    for color in ["#020126", "#5b498a", "#592004"]:
        assert color in raga_css, f"Indian Raga palette missing {color}"
    for old_color in ["#2b1610", "#5e2319", "#d4aa4d", "#f9df9b", "#8d1f17"]:
        assert old_color not in raga_css, f"Indian Raga still contains old palette {old_color}"
    assert "Indian Raga" in text

    width = re.search(r"--w:\s*(\d+)px", text)
    height = re.search(r"--h:\s*(\d+)px", text)
    assert width and int(width.group(1)) <= 220
    assert height and int(height.group(1)) <= 370
    assert re.search(r"border-radius:\s*22px", text)
    print("web_simulator_checks passed")


if __name__ == "__main__":
    main()
