# SHAeR Memory, CPU, And Power Budgets V1

Status: Initial budget for Raspberry Pi Zero 2 W. Must be measured on hardware and updated with real numbers.

## Memory Budget

Target RAM ceiling during playback: under 320 MB.

| Area | Budget |
|---|---:|
| Firmware core/AppState/services | 25 MB |
| Renderer/display buffers | 40 MB |
| Theme assets/icons/fonts | 35 MB |
| Album art cache in memory | 80 MB |
| SQLite page/cache | 20 MB |
| Audio buffers/decoder | 45 MB |
| Spotify/librespot process | 120 MB |
| Python helper services when active | 80 MB |

## CPU Budget

| Mode | Target Average CPU |
|---|---:|
| Idle Home | under 12% |
| Local playback, static screen | under 25% |
| Local playback, animated Now Playing | under 40% |
| Spotify Connect playback | under 55% |
| Bluetooth reconnect | under 35% |
| Voice recording | under 35% |
| Album art decode/sync | background only; throttle if audio active |
| Charging/AOD | under 8% |

## Power Policy

Battery saver must:

- Cap UI to 12 fps.
- Reduce animated elements to 2.
- Disable non-essential drift/particles.
- Prefer local playback over network retries when Wi-Fi is unstable.

Critical battery must:

- Cap UI to 8 fps.
- Keep only essential animations.
- Show clear shutdown/recharge guidance.

