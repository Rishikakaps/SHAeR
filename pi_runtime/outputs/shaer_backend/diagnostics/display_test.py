#!/usr/bin/env python3
import os
from pathlib import Path


OUTPUTS = Path(__file__).resolve().parents[2]
THEMES = (
    "shaer_base_dark", "shaer_base_light",
    "shaer_dark_archive", "shaer_bombay_ticket", "shaer_japanese_punk",
    "shaer_windows_xp", "shaer_ghibli_garden", "shaer_indian_print",
)


def main():
    for theme in THEMES:
        assert (OUTPUTS / theme / "index.html").is_file()
    assert (OUTPUTS / "shaer_pi_os" / "system-overlays.css").is_file()
    if os.environ.get("SHAER_HARDWARE") == "1":
        assert Path("/dev/spidev0.0").exists() or Path("/dev/fb0").exists(), "ILI9341 SPI/framebuffer device not found"
        print("display_test ok mode=hardware")
    else:
        print("display_test ok mode=contract physical=pending")


if __name__ == "__main__": main()
