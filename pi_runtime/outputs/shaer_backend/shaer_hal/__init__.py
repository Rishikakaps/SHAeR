"""Hardware abstraction contracts for SHAeR firmware."""

from .interfaces import (
    AudioDevice,
    BatteryDevice,
    BluetoothDevice,
    DisplayDevice,
    HardwareAbstractionLayer,
    MicrophoneDevice,
    NetworkDevice,
    PowerDevice,
)
from .input import GpioInputController, InputAction, InputEvent, SimulatedInputController

__all__ = [
    "AudioDevice",
    "BatteryDevice",
    "BluetoothDevice",
    "DisplayDevice",
    "GpioInputController",
    "HardwareAbstractionLayer",
    "InputAction",
    "InputEvent",
    "MicrophoneDevice",
    "NetworkDevice",
    "PowerDevice",
    "SimulatedInputController",
]
