from __future__ import annotations

from pathlib import Path

from .models import ImportResult
from .providers.base import ImportProvider
from .providers.local_folder import LocalFolderProvider


class PluginManager:
    def __init__(self) -> None:
        self._providers: dict[str, ImportProvider] = {}
        self.register_provider(LocalFolderProvider())

    def register_provider(self, provider: ImportProvider) -> None:
        self._providers[provider.provider_id] = provider

    def providers(self) -> list[ImportProvider]:
        return list(self._providers.values())

    def provider(self, provider_id: str) -> ImportProvider:
        return self._providers[provider_id]

    def import_from(self, provider_id: str, source: Path) -> ImportResult:
        return self.provider(provider_id).scan(source)

