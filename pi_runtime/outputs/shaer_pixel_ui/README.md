# SHAeR Pixel UI Prototype

This is a runnable UI/theme prototype for SHAeR, focused on the real product screen rather than test scaffolding.

## Run

From this folder:

```bash
python3 -m http.server 8765
```

Open:

```text
http://127.0.0.1:8765/
```

## Included

- `index.html` - app shell and controls.
- `src/styles.css` - responsive device frame and control surface.
- `src/themes.js` - renderer-facing theme data used by the live canvas app.
- `src/app.js` - 240x320 canvas renderer, navigation, screen state, keyboard/control handling.
- `themes/*.json` - package-style theme contracts for the six visual worlds.

## Current Screens

- Boot
- Home
- Library
- Now Playing
- Settings
- Spotify Drop popup
- Charging
- Sleep / AOD

## Controls

- Click the physical buttons below the device frame.
- Use theme and screen controls in the right/bottom panel.
- Keyboard works too: ArrowUp, ArrowDown, Enter, Escape/Backspace.

## Design Intent

The canvas is fixed at 240x320, matching the SHAeR display target. The browser scales it with pixel rendering, so layout mistakes are visible at the same aspect ratio as the device.

The default Home selection is Local Files, keeping the prototype aligned with the local-first layer before Spotify is implemented.
