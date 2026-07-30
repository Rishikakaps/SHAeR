"""Protocols separating firmware behavior from Raspberry Pi drivers."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol


class DisplayDevice(Protocol):
    def present(self, frame: bytes) -> None: ...


class AudioDevice(Protocol):
    def set_volume(self, percent: int) -> None: ...
    def stop(self) -> None: ...


class BatteryDevice(Protocol):
    def snapshot(self) -> dict[str, object]: ...


class NetworkDevice(Protocol):
    def snapshot(self) -> dict[str, object]: ...


class BluetoothDevice(Protocol):
    def snapshot(self) -> dict[str, object]: ...


class MicrophoneDevice(Protocol):
    def available(self) -> bool: ...


class PowerDevice(Protocol):
    def shutdown(self) -> None: ...


@dataclass(slots=True)
class HardwareAbstractionLayer:
    """Injected hardware dependencies. Unsupported hardware remains explicit."""

    display: DisplayDevice | None = None
    audio: AudioDevice | None = None
    battery: BatteryDevice | None = None
    network: NetworkDevice | None = None
    bluetooth: BluetoothDevice | None = None
    microphone: MicrophoneDevice | None = None
    power: PowerDevice | None = None

    def capabilities(self) -> dict[str, bool]:
        return {
            "display": self.display is not None,
            "audio": self.audio is not None,
            "battery": self.battery is not None,
            "network": self.network is not None,
            "bluetooth": self.bluetooth is not None,
            "microphone": self.microphone is not None,
            "power": self.power is not None,
        }
