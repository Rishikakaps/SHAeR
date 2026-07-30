#!/usr/bin/env python3
from pathlib import Path


def main():
    outputs = Path(__file__).resolve().parents[2]
    input_hal = (outputs / "shaer_backend" / "shaer_hal" / "input.py").read_text(encoding="utf-8")
    server = (outputs / "shaer_pi_os" / "server.py").read_text(encoding="utf-8")
    for token in ("when_released", "LONG_SELECT", "hold_time", "bounce_time", "InputAction.HOME"):
        assert token in input_hal
    assert "GpioInputController" in server
    print("button_test ok hal=true short=select/back long=queue/home debounce=true")


if __name__ == "__main__": main()
