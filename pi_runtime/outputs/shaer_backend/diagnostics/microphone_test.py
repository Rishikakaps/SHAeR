#!/usr/bin/env python3
import os
import shutil
import subprocess

from shaer_recording import GStreamerCaptureBackend


def main():
    backend = GStreamerCaptureBackend(os.environ.get("SHAER_MIC_DEVICE", "hw:MIC,0"))
    assert backend.device
    if os.environ.get("SHAER_HARDWARE") == "1":
        assert shutil.which("gst-launch-1.0"), "gst-launch-1.0 is required on the Raspberry Pi"
        assert shutil.which("arecord"), "arecord is required to enumerate the microphone"
        result = subprocess.run(["arecord", "-l"], check=True, capture_output=True, text=True)
        assert "card" in result.stdout.lower(), "No ALSA capture card was found"
        print(f"microphone_test ok hardware=true device={backend.device}")
        return
    print(f"microphone_test ok hardware=PENDING device={backend.device} backend=gstreamer")


if __name__ == "__main__":
    main()
