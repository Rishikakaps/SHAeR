#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    themes = (
        "shaer_base_dark", "shaer_base_light",
        "shaer_dark_archive", "shaer_bombay_ticket", "shaer_japanese_punk",
        "shaer_windows_xp", "shaer_ghibli_garden", "shaer_indian_print",
    )
    sources = "\n".join(
        path.read_text(encoding="utf-8")
        for theme in themes
        for path in (outputs / theme / "src").glob("*.js")
    )
    assert 'data-target="folders"' not in sources
    assert 'addEventListener("keydown"' not in sources
    assert "function goTo(" not in sources
    core = (outputs / "shaer_pi_os" / "firmware-core.js").read_text(encoding="utf-8")
    for token in ("function transition(", "function back(", "function normalizeState(", "function moveSelection("):
        assert token in core
    bridge = (outputs / "shaer_pi_os" / "hardware-bridge.js").read_text(encoding="utf-8")
    for action in ("left", "right", "select", "back", "home", "long_select"):
        assert action in bridge
    print("navigation_test ok core=shared state_machine=central back=hardware")


if __name__ == "__main__": main()
