# SHAeR Theme API V1

Status: V1 freeze draft.

## Purpose

Themes provide data. Firmware consumes that data. Theme behavior must not be hard-coded into navigation, playback, power, or recovery logic.

## Theme Folder Layout

```text
themes/
  archive_dark/
    theme.json
    fonts/
    icons/
    images/
    animations/
    sounds/
    layouts/
  bombay_ticket/
  indian_raga/
  windows_xp/
  japanese_punk/
  ghibli_garden/
```

## theme.json Required Fields

```json
{
  "api_version": "ThemeAPI.v1",
  "id": "indian_raga",
  "display_name": "Indian Raga",
  "emotions": {
    "boot": "ceremony and craft",
    "now_playing": "calm immersion",
    "charging": "warmth and craftsmanship",
    "settings": "nostalgia",
    "errors": "gentle guidance"
  },
  "fonts": {},
  "colors": {},
  "layout": {},
  "transitions": {},
  "animations": {},
  "screens": {},
  "icon_pack": {},
  "sound_effects": {}
}
```

## Required Theme Capabilities

Each theme provides:

- Fonts.
- Colors.
- Layout.
- Transitions.
- Animations.
- Boot screen.
- Loading screen.
- Home screen.
- Now Playing screen.
- Local Library screen.
- Memory Mode screen.
- Voice Recording screen.
- Charging screen.
- Popup style.
- Progress bar.
- AOD style.
- Icon pack.
- Sound effects.

## Color Rules

Theme colors must be named semantic tokens:

- `background`.
- `surface`.
- `text_primary`.
- `text_muted`.
- `accent`.
- `accent_secondary`.
- `warning`.
- `success`.
- `battery_low`.

Indian Raga V1:

- Background: deep indigo blue.
- Secondary: lavender.
- Highlight: burnt sienna.
- Do not use red/yellow as the primary identity.

## Layout Rules

Each screen has a blueprint:

- Chrome.
- Primary region.
- Selector.
- Header behavior.
- Footer behavior.
- Safe text regions.
- Album art position.
- Progress position.

Themes must differ structurally, not only by palette.

## Animation Rules

Theme provides preferred animation. App power policy decides final budget.

Theme fields:

- Normal target FPS.
- Maximum rich animated elements.
- Transition signatures.
- Loading signature.
- AOD signature.
- Charging signature.

Theme must not contain battery thresholds.

## Sound Effects

Sound effects are optional and must be disabled by default during:

- Recording.
- Critical battery.
- Shutdown after commit point.
- User mute mode.

## Versioning

ThemeAPI v1 is frozen for SHAeR V1. V2 changes require:

- New `api_version`.
- Migration notes.
- Compatibility loader or explicit unsupported-theme message.

