from __future__ import annotations

import json
import shutil
from pathlib import Path

from .events import Event, EventBus


class ThemeManager:
    def __init__(self, themes_dir: Path, events: EventBus) -> None:
        self.themes_dir = themes_dir
        self.events = events
        self.themes_dir.mkdir(parents=True, exist_ok=True)

    def install_theme(self, source_dir: Path) -> Path:
        manifest = source_dir / "theme.json"
        if not manifest.exists():
            raise ValueError(f"theme.json missing in {source_dir}")
        data = json.loads(manifest.read_text(encoding="utf-8"))
        theme_id = data.get("id") or source_dir.name
        destination = self.themes_dir / str(theme_id)
        if destination.exists():
            shutil.rmtree(destination)
        shutil.copytree(source_dir, destination)
        self.events.publish(Event("theme.installed", "ThemeManager", {"theme_id": str(theme_id)}))
        return destination

    def themes(self) -> list[dict]:
        themes: list[dict] = []
        for manifest in sorted(self.themes_dir.glob("*/theme.json")):
            themes.append(json.loads(manifest.read_text(encoding="utf-8")))
        return themes

    def customize_colors(self, theme_id: str, colors: dict[str, str]) -> None:
        manifest = self.themes_dir / theme_id / "theme.json"
        data = json.loads(manifest.read_text(encoding="utf-8"))
        data.setdefault("colors", {}).update(colors)
        manifest.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
        self.events.publish(Event("theme.colors.changed", "ThemeManager", {"theme_id": theme_id}))

