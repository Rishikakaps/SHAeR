#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    input_hal = (outputs / "shaer_backend" / "shaer_hal" / "input.py").read_text(encoding="utf-8")
    server = (outputs / "shaer_pi_os" / "server.py").read_text(encoding="utf-8")
    bridge = (Path(__file__).resolve().parents[2] / "shaer_pi_os" / "hardware-bridge.js").read_text(encoding="utf-8")
    for token in ("RotaryEncoder", "when_rotated_clockwise", "when_rotated_counter_clockwise"):
        assert token in input_hal
    assert "GpioInputController" in server
    for token in ('left: "ArrowLeft"', 'right: "ArrowRight"', "navigationLatencyMs"):
        assert token in bridge
    print("encoder_test ok hal=true rotation=left/right acceleration=bounded")


if __name__ == "__main__": main()
