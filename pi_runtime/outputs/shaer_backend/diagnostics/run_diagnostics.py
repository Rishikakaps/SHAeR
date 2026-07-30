#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path


DIAGNOSTICS = [
    "sqlite_test.py",
    "library_test.py",
    "queue_test.py",
    "statistics_test.py",
    "microphone_test.py",
    "recording_test.py",
    "playback_test.py",
    "metadata_test.py",
    "waveform_test.py",
    "spotify_matching_test.py",
    "spotify_login_test.py",
    "spotify_api_test.py",
    "spotify_connect_test.py",
    "spotify_cache_test.py",
    "spotify_queue_test.py",
    "spotify_transfer_test.py",
    "spotify_reconnect_test.py",
    "spotify_metadata_test.py",
    "display_test.py",
    "encoder_test.py",
    "button_test.py",
    "audio_test.py",
    "theme_test.py",
    "renderer_test.py",
    "navigation_test.py",
    "storage_test.py",
    "gpio_test.py",
    "battery_test.py",
    "dac_test.py",
    "charging_test.py",
    "bluetooth_test.py",
    "wifi_test.py",
    "memory_test.py",
    "cpu_test.py",
    "temperature_test.py",
]


def main() -> None:
    here = Path(__file__).resolve().parent
    for name in DIAGNOSTICS:
        subprocess.run([sys.executable, str(here / name)], check=True)


if __name__ == "__main__":
    main()
