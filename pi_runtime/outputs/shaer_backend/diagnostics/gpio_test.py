#!/usr/bin/env python3
import os
from pathlib import Path


def main():
    server = (Path(__file__).resolve().parents[2] / "shaer_pi_os" / "server.py").read_text(encoding="utf-8")
    for pin in ("default=17", "default=27", "default=22", "default=23"):
        assert pin in server
    if os.environ.get("SHAER_HARDWARE") == "1":
        assert Path("/sys/class/gpio").exists(), "GPIO sysfs unavailable"
        print("gpio_test ok mode=hardware")
    else:
        print("gpio_test ok mode=contract pins=17,27,22,23")


if __name__ == "__main__": main()
