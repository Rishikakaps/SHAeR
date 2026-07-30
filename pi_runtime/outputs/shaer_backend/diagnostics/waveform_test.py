#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    bridge = (outputs / "shaer_pi_os" / "hardware-bridge.js").read_text(encoding="utf-8")
    css = (outputs / "shaer_pi_os" / "system-overlays.css").read_text(encoding="utf-8")
    for selector in (".memo-bars i", ".waveform i", ".memo-wave i"):
        assert selector in bridge
    assert "scaleY" in bridge
    assert "shaer-record-pulse" in css
    print("waveform_test ok live_level_binding=true recording_animation=true")


if __name__ == "__main__":
    main()
