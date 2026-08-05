from __future__ import annotations

from collections import defaultdict, deque
from dataclasses import dataclass, field
from threading import RLock
from time import time
from typing import Any, Callable, DefaultDict, Deque


@dataclass(frozen=True)
class Event:
    name: str
    source: str
    payload: dict[str, Any] = field(default_factory=dict)
    created_at: float = field(default_factory=time)


Subscriber = Callable[[Event], None]


class EventBus:
    def __init__(self) -> None:
        self._subscribers: DefaultDict[str, list[Subscriber]] = defaultdict(list)
        self._queue: Deque[Event] = deque()
        self._lock = RLock()

    def subscribe(self, event_name: str, callback: Subscriber) -> None:
        with self._lock:
            self._subscribers[event_name].append(callback)

    def publish(self, event: Event) -> None:
        with self._lock:
            self._queue.append(event)

    def drain(self) -> list[Event]:
        with self._lock:
            events = list(self._queue)
            self._queue.clear()
        for event in events:
            callbacks = list(self._subscribers.get(event.name, ()))
            callbacks += list(self._subscribers.get("*", ()))
            for callback in callbacks:
                callback(event)
        return events

    def pending_count(self) -> int:
        with self._lock:
            return len(self._queue)

