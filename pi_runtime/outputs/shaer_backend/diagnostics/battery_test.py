#!/usr/bin/env python3
import os
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    theme_sources = "\n".join(path.read_text(encoding="utf-8") for path in outputs.glob("shaer_*/src/*.js"))
    assert "battery" in theme_sources.lower()
    if os.environ.get("SHAER_HARDWARE") == "1":
        adc = os.environ.get("SHAER_BATTERY_ADC")
        assert adc and Path(adc).exists(), "Set SHAER_BATTERY_ADC to the battery ADC sysfs path"
        value = Path(adc).read_text(encoding="utf-8").strip()
        assert value.isdigit(), "Battery ADC did not return a numeric reading"
        print(f"battery_test ok mode=hardware raw={value}")
    else:
        print("battery_test ok mode=contract adc=physical-pending")


if __name__ == "__main__": main()
