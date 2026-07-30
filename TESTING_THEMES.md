# SHAeR Theme Testing

## Local Preview

```sh
cd /Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
python3 pi_runtime/outputs/shaer_pi_os/server.py --host 127.0.0.1 --port 8790
```

Preview URL:

```text
http://127.0.0.1:8790/shaer_dark_archive/?mode=device&diagnostic=spotify&validation=1
```

Replace `shaer_dark_archive` with `shaer_base_dark`, `shaer_base_light`, `shaer_bombay_ticket`, `shaer_japanese_punk`, `shaer_windows_xp`, `shaer_ghibli_garden`, or `shaer_indian_print`.

## Physical-Control Keyboard Map

```text
ArrowLeft   previous encoder item
ArrowRight  next encoder item
Enter       select
Backspace   back
Escape      home / close overlay
+ or =      volume up
- or _      volume down
b           Bluetooth connection screen
```

## State Simulation

Use validation mode plus the diagnostic query:

```text
?mode=device&diagnostic=spotify&validation=1
?mode=device&diagnostic=local&validation=1
```

The automated validator drives these states through `window.shaerValidation.render(state)`: `boot`, `home`, `library`, `album`, `now-playing`, `recording`, `volume`, `queue`, `bluetooth`, `spotify-login`, `loading`, `error`, and `shutdown`.

## Commands

```sh
node pi_runtime/outputs/theme_validation/theme-validation.mjs --no-baseline
node pi_runtime/outputs/theme_validation/settings-navigation-check.mjs
node pi_runtime/outputs/theme_validation/contact-sheet.mjs
python3 pi_runtime/outputs/shaer_backend/diagnostics/theme_test.py
python3 pi_runtime/outputs/shaer_backend/diagnostics/display_test.py
python3 pi_runtime/outputs/shaer_backend/diagnostics/renderer_test.py
python3 pi_runtime/outputs/shaer_backend/diagnostics/navigation_test.py
```

The contact sheet is written to:

```text
pi_runtime/outputs/theme_validation/artifacts/theme-contact-sheet.png
```

Hardware-only acceptance remains separate from host validation. Battery health, charge cycles, RTC, Bluetooth hardware state, microphone capture, DAC output, and physical Pi controls must be verified on the device before being marked complete.
