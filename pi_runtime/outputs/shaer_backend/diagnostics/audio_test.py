#!/usr/bin/env python3
import os
import shutil
import subprocess
from pathlib import Path


def main():
    service = (Path(__file__).resolve().parents[2] / "shaer_pi_os" / "shaer-librespot.service").read_text(encoding="utf-8")
    assert "--backend alsa" in service and "SHAER_ALSA_DEVICE" in service
    if os.environ.get("SHAER_HARDWARE") == "1":
        assert shutil.which("aplay"), "aplay is not installed"
        result = subprocess.run(["aplay", "-l"], capture_output=True, text=True, check=False)
        assert result.returncode == 0 and "card" in result.stdout.lower(), "No ALSA playback device found"
        print("audio_test ok mode=hardware alsa=true")
    else:
        print("audio_test ok mode=contract pcm5102=physical-pending")


if __name__ == "__main__": main()
