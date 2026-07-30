#!/usr/bin/env python3
from pathlib import Path


OUTPUTS = Path(__file__).resolve().parents[2]
THEMES = (
    "shaer_base_dark", "shaer_base_light",
    "shaer_dark_archive", "shaer_bombay_ticket", "shaer_japanese_punk",
    "shaer_windows_xp", "shaer_ghibli_garden", "shaer_indian_print",
)


def main():
    for theme in THEMES:
        html = (OUTPUTS / theme / "index.html").read_text(encoding="utf-8")
        assert "system-overlays.css" in html
        assert "firmware-core.js" in html
        assert "hardware-bridge.js" in html
        assert html.index("firmware-core.js") < html.index("hardware-bridge.js")
        assert 'id="liveScreen"' in html
        scripts = "\n".join(path.read_text(encoding="utf-8") for path in (OUTPUTS / theme / "src").glob("*.js"))
        assert "recording-library" in scripts
        assert "toggle-memo" in scripts
        assert 'recordings: "memos"' in scripts
    print(f"theme_test ok themes={len(THEMES)} shared_core=true visual_adapters=true")


if __name__ == "__main__": main()
