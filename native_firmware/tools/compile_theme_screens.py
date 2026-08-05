#!/usr/bin/env python3
"""Compile supplied theme reference sheets into Pi-ready RGB565 screens.

The input files are contact sheets with eight portrait screens on a dark
background. This script crops each detected screen, fits it to the 240x320 TFT
without changing aspect ratio, and writes:

  assets/themes/<theme>/screens/<screen>.rgb565
  assets/themes/<theme>/screens/<screen>.png
"""

from __future__ import annotations

from pathlib import Path
from typing import Iterable

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "assets" / "reference_sheets"
THEME_DIR = ROOT / "assets" / "themes"
SCREEN_SIZE = (240, 320)

THEME_FILES = {
    "archive_dark": "archive_dark.png",
    "bombay_ticket": "bombay_ticket.png",
    "japanese_punk": "japanese_punk.png",
    "windows_xp": "windows_xp.png",
    "ghibli_garden": "ghibli_garden.png",
    "indian_raga": "indian_raga.png",
}

SCREEN_MAP = {
    "archive_dark": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
    "bombay_ticket": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
    "japanese_punk": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
    "windows_xp": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
    "ghibli_garden": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
    "indian_raga": {
        "home": 0,
        "boot": 1,
        "library": 2,
        "about": 3,
        "now_playing": 4,
        "voice_archive": 5,
        "settings": 6,
        "charging": 7,
    },
}


def column_segments(image: Image.Image) -> list[tuple[int, int]]:
    rgb = image.convert("RGB")
    background = rgb.getpixel((0, 0))
    active_columns: list[int] = []
    for x in range(rgb.width):
        count = 0
        for y in range(rgb.height):
            r, g, b = rgb.getpixel((x, y))
            if abs(r - background[0]) + abs(g - background[1]) + abs(b - background[2]) > 25:
                count += 1
        if count > 5:
            active_columns.append(x)

    if not active_columns:
        return []

    segments: list[tuple[int, int]] = []
    start = previous = active_columns[0]
    for x in active_columns[1:]:
        if x - previous > 5:
            if previous - start > 20:
                segments.append((start, previous))
            start = x
        previous = x
    if previous - start > 20:
        segments.append((start, previous))
    return segments


def vertical_bounds(image: Image.Image, x0: int, x1: int) -> tuple[int, int]:
    rgb = image.convert("RGB")
    background = rgb.getpixel((0, 0))
    rows: list[int] = []
    for y in range(rgb.height):
        count = 0
        for x in range(x0, x1 + 1):
            r, g, b = rgb.getpixel((x, y))
            if abs(r - background[0]) + abs(g - background[1]) + abs(b - background[2]) > 25:
                count += 1
        if count > 5:
            rows.append(y)
    if not rows:
        return 0, rgb.height - 1
    return min(rows), max(rows)


def fit_to_tft(crop: Image.Image) -> Image.Image:
    crop = crop.convert("RGB")
    scale = min(SCREEN_SIZE[0] / crop.width, SCREEN_SIZE[1] / crop.height)
    fitted_size = (
        max(1, round(crop.width * scale)),
        max(1, round(crop.height * scale)),
    )
    crop = crop.resize(fitted_size, Image.Resampling.LANCZOS)
    background = crop.getpixel((0, 0))
    canvas = Image.new("RGB", SCREEN_SIZE, background)
    x = (SCREEN_SIZE[0] - crop.width) // 2
    y = (SCREEN_SIZE[1] - crop.height) // 2
    canvas.paste(crop, (x, y))
    return canvas


def rgb565_bytes(image: Image.Image) -> bytes:
    out = bytearray()
    rgb = image.convert("RGB")
    pixels: Iterable[tuple[int, int, int]]
    if hasattr(rgb, "get_flattened_data"):
        pixels = rgb.get_flattened_data()  # type: ignore[attr-defined]
    else:
        pixels = rgb.getdata()
    for r, g, b in pixels:
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out.append((value >> 8) & 0xFF)
        out.append(value & 0xFF)
    return bytes(out)


def compile_theme(theme: str, filename: str) -> None:
    source = SOURCE_DIR / filename
    if not source.exists():
        raise FileNotFoundError(source)

    sheet = Image.open(source).convert("RGBA")
    segments = column_segments(sheet)
    if len(segments) < 8:
        raise RuntimeError(f"{source} produced only {len(segments)} crop segments")

    crops: list[Image.Image] = []
    for x0, x1 in segments[:8]:
        y0, y1 = vertical_bounds(sheet, x0, x1)
        crops.append(sheet.crop((x0, y0, x1 + 1, y1 + 1)))

    out_dir = THEME_DIR / theme / "screens"
    out_dir.mkdir(parents=True, exist_ok=True)

    for screen, index in SCREEN_MAP[theme].items():
        fitted = fit_to_tft(crops[index])
        fitted.save(out_dir / f"{screen}.png")
        (out_dir / f"{screen}.rgb565").write_bytes(rgb565_bytes(fitted))

    print(f"{theme}: compiled {len(SCREEN_MAP[theme])} screens -> {out_dir}")


def main() -> int:
    for theme, filename in THEME_FILES.items():
        compile_theme(theme, filename)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
