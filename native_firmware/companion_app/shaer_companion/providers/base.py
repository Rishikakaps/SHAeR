from __future__ import annotations

from abc import ABC, abstractmethod
from pathlib import Path

from ..models import ImportResult


class ImportProvider(ABC):
    provider_id: str
    display_name: str

    @abstractmethod
    def scan(self, source: Path) -> ImportResult:
        raise NotImplementedError

