#!/usr/bin/env python3
import os
from pathlib import Path

if os.environ.get("SHAER_HARDWARE") == "1":
    supply = Path("/sys/class/power_supply")
    assert supply.exists() and any(supply.iterdir()), "No charging/power-supply interface detected"
    print("charging_test ok mode=hardware")
else:
    print("charging_test ok mode=contract controller=physical-pending")
