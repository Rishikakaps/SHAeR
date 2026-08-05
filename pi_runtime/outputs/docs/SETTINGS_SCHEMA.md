# SHAeR Settings Schema v1

Settings are stored atomically in `~/.config/shaer/settings.json`. `POST /api/v1/settings` accepts a partial category patch and rejects unknown categories or keys.

| Category | Keys |
| --- | --- |
| `audio` | `volume_limit`, `equalizer_preset`, `bass`, `mid`, `treble`, `loudness`, `replay_gain`, `crossfade_s`, `channels` |
| `display` | `brightness`, `sleep_timeout_s`, `animation_speed`, `theme`, `clock_format` |
| `bluetooth` | `device_name`, `pairing`, `auto_connect`, `codec` |
| `wifi` | `dhcp`, `hostname`, `saved_networks` |
| `power` | `auto_shutdown_min`, `low_battery_percent`, `charging_behavior` |
| `library` | `scan_paths`, `automatic_rescan`, `artwork_cache_mb` |
| `spotify` | `device_name` |
| `privacy` | `listening_history`, `analytics` |
| `developer` | `ssh`, `logging_level`, `debug_overlays` |

The companion renders controls from this tree and sends each changed value immediately. Spotify login/logout remains in the existing Spotify service; secrets and access tokens are never returned through settings.

Changing `display.theme` through the dedicated theme endpoint persists the setting and emits a live theme-switch event to the device renderer.

