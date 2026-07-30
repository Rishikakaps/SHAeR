"""Encoder and push-button input devices behind one event callback."""

from __future__ import annotations

import time
from dataclasses import dataclass
from enum import StrEnum
from threading import Timer
from typing import Callable


class InputAction(StrEnum):
    LEFT = "left"
    RIGHT = "right"
    SELECT = "select"
    BACK = "back"
    HOME = "home"
    LONG_SELECT = "long_select"
    TOGGLE_INPUT_MODE = "toggle_input_mode"


@dataclass(frozen=True, slots=True)
class InputEvent:
    action: InputAction
    source: str
    monotonic_time: float


InputCallback = Callable[[InputEvent], None]


class SimulatedInputController:
    def __init__(self, callback: InputCallback):
        self.callback = callback

    def emit(self, action: InputAction) -> None:
        self.callback(InputEvent(action, "simulated", time.monotonic()))

    def close(self) -> None:
        return


class GpioInputController:
    """Owns gpiozero resources and emits debounced semantic input actions."""

    def __init__(
        self,
        callback: InputCallback,
        *,
        pin_a: int,
        pin_b: int,
        pin_select: int,
        pin_back: int,
        pin_home: int = -1,
        bounce_time: float = 0.035,
        hold_time: float = 0.8,
        double_click_window: float = 0.32,
    ):
        self.callback = callback
        self.pin_a = pin_a
        self.pin_b = pin_b
        self.pin_select = pin_select
        self.pin_back = pin_back
        self.pin_home = pin_home
        self.bounce_time = bounce_time
        self.hold_time = hold_time
        self.double_click_window = double_click_window
        self.devices: list[object] = []
        self._press_started: dict[str, float] = {}
        self._pending_select: Timer | None = None

    def start(self) -> None:
        from gpiozero import Button, RotaryEncoder

        encoder = RotaryEncoder(self.pin_a, self.pin_b, max_steps=0)
        select = Button(self.pin_select, pull_up=True, bounce_time=self.bounce_time)
        back = Button(self.pin_back, pull_up=True, bounce_time=self.bounce_time)
        home = Button(self.pin_home, pull_up=True, bounce_time=self.bounce_time) if self.pin_home >= 0 else None

        encoder.when_rotated_clockwise = lambda: self._emit(InputAction.RIGHT)
        encoder.when_rotated_counter_clockwise = lambda: self._emit(InputAction.LEFT)
        select.when_pressed = lambda: self._pressed("select")
        select.when_released = lambda: self._released("select", InputAction.SELECT, InputAction.LONG_SELECT)
        back.when_pressed = lambda: self._pressed("back")
        back.when_released = lambda: self._released("back", InputAction.BACK, InputAction.HOME)
        if home:
            home.when_pressed = lambda: self._emit(InputAction.HOME)
        self.devices = [device for device in (encoder, select, back, home) if device]

    def _emit(self, action: InputAction) -> None:
        self.callback(InputEvent(action, "gpio", time.monotonic()))

    def _pressed(self, name: str) -> None:
        self._press_started[name] = time.monotonic()

    def _released(self, name: str, short: InputAction, long: InputAction) -> None:
        duration = time.monotonic() - self._press_started.pop(name, time.monotonic())
        if duration >= self.hold_time:
            if name == "select" and self._pending_select:
                self._pending_select.cancel()
                self._pending_select = None
            self._emit(long)
            return
        if name != "select":
            self._emit(short)
            return
        if self._pending_select and self._pending_select.is_alive():
            self._pending_select.cancel()
            self._pending_select = None
            self._emit(InputAction.TOGGLE_INPUT_MODE)
            return
        self._pending_select = Timer(self.double_click_window, self._emit_delayed_select)
        self._pending_select.daemon = True
        self._pending_select.start()

    def _emit_delayed_select(self) -> None:
        self._pending_select = None
        self._emit(InputAction.SELECT)

    def close(self) -> None:
        if self._pending_select:
            self._pending_select.cancel()
            self._pending_select = None
        for device in self.devices:
            close = getattr(device, "close", None)
            if callable(close):
                close()
        self.devices.clear()
