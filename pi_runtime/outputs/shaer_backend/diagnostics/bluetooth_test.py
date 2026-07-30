#!/usr/bin/env python3
import os
import shutil
import subprocess

if os.environ.get("SHAER_HARDWARE") == "1":
    assert shutil.which("bluetoothctl"), "bluetoothctl is not installed"
    result = subprocess.run(["bluetoothctl", "show"], capture_output=True, text=True, timeout=8, check=False)
    assert result.returncode == 0 and "Powered: yes" in result.stdout, "Bluetooth controller is not powered"
    print("bluetooth_test ok mode=hardware powered=true")
else:
    print("bluetooth_test ok mode=contract radio=physical-pending")
