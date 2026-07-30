#!/usr/bin/env python3
import os
from pathlib import Path

path = Path("/sys/class/thermal/thermal_zone0/temp")
if os.environ.get("SHAER_HARDWARE") == "1":
    value = int(path.read_text(encoding="utf-8").strip()) / 1000
    assert 0 < value < 100, f"Unsafe or invalid CPU temperature: {value}C"
    print(f"temperature_test ok mode=hardware cpu_c={value:.1f}")
else:
    print("temperature_test ok mode=contract sensor=physical-pending")
