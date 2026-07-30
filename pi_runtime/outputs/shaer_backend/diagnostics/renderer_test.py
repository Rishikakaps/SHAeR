#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    files = [
        outputs / "shaer_base_dark" / "index.html",
        outputs / "shaer_base_dark" / "src" / "base-theme.js",
        outputs / "shaer_base_light" / "index.html",
        outputs / "shaer_base_light" / "src" / "base-theme.js",
        outputs / "shaer_dark_archive" / "index.html",
        outputs / "shaer_bombay_ticket" / "src" / "bombay-ticket.js",
        outputs / "shaer_japanese_punk" / "src" / "japanese-punk.js",
        outputs / "shaer_windows_xp" / "src" / "windows-xp.js",
        outputs / "shaer_ghibli_garden" / "src" / "ghibli-garden.js",
        outputs / "shaer_indian_print" / "src" / "indian-print.js",
    ]
    for path in files:
        source = path.read_text(encoding="utf-8")
        assert "data-shaer-title" in source and "data-shaer-progress" in source
    bridge = (outputs / "shaer_pi_os" / "hardware-bridge.js").read_text(encoding="utf-8")
    assert "applyPlayback" in bridge and "if (spotify)" not in bridge
    assert 'source: "recording"' in bridge
    assert "showRecordingLibrary" in bridge
    print("renderer_test ok themes=8 unified_playback=true recording_archive=true")


if __name__ == "__main__": main()
