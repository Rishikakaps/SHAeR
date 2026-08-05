# SHAeR Recovery Modes V1

Status: V1 freeze draft.

## Purpose

Recovery Mode is for fixing hardware and software problems without panic. It should feel plain, capable, and reassuring.

## Entry Points

- Settings -> Recovery.
- Diagnostic button chord during boot.
- Automatic entry after repeated boot failure.
- Automatic entry after corrupted settings/database recovery failure.

## Required Modes

| Mode | What It Tests | Pass Criteria |
|---|---|---|
| Display test | Backlight, colors, text, clipping | All patterns visible and readable |
| Button test | Every physical button | Presses appear once, no stuck state |
| Encoder test | Encoder A/B and push | Direction correct, no bounce storms |
| Audio test | DAC, ALSA, 3.5 mm output | Left/right test tones play cleanly |
| Battery status | MAX17048, charger state, voltage fallback | Percent and charging state readable |
| Bluetooth test | BlueZ, known headphones, reconnect | Known headphones reconnect or useful error appears |
| Wi-Fi test | Interface, saved network, DNS/ping | Network status shown without blocking UI |
| SD card test | Free space, read/write, DB access | Test file and SQLite check succeed |
| Factory reset | Settings/database reset | Requires confirmation and preserves optional user media if selected |
| Export logs | Copy logs/database/settings | Export archive appears on companion/USB/storage target |

## Recovery UI

- Use simple list navigation.
- Avoid theme-heavy animation.
- Show one result at a time.
- Each failed test gives a next action.
- No raw stack trace unless user opens advanced details.

## Factory Reset

Factory reset must offer:

- Reset settings only.
- Reset settings and database.
- Full wipe including local cache.

Voice memories and local music should never be deleted without explicit confirmation.

## Export Bundle

Export logs should create:

```text
shaer-diagnostics-YYYYMMDD-HHMM/
  logs/
  settings/
  database/
  hardware_report.txt
  firmware_report.txt
```

Secrets must be redacted.

