#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    bridge = (outputs / "shaer_pi_os" / "hardware-bridge.js").read_text(encoding="utf-8")
    server = (outputs / "shaer_pi_os" / "server.py").read_text(encoding="utf-8")
    assert 'source: "recording"' in bridge
    assert "new Audio(`/api/recording/audio/" in bridge
    assert "applyPlayback(runtime.playback)" in bridge
    assert 'path.startswith("/api/recording/audio/")' in server
    assert 'disposition = "attachment" if download else "inline"' in server
    print("playback_test ok source_neutral_ui=true inline_wav=true dac=PENDING")


if __name__ == "__main__":
    main()
