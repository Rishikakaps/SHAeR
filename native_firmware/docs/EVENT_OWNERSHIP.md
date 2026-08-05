# SHAeR Event Ownership V1

Status: Required before hardware testing.

No service may directly manipulate another service. Services publish events; state mutation happens through controlled handlers.

| Event | Publisher | Owner/Handler | Result |
|---|---|---|---|
| `BUTTON_OK_PRESSED` | InputService | NavigationService | Confirm focused action |
| `BUTTON_BACK_PRESSED` | InputService | NavigationService | Pop navigation stack |
| `NAVIGATE_UP/DOWN` | InputService | NavigationService | Change selected index |
| `PLAY_SELECTED_TRACK_REQUESTED` | NavigationService | AudioService | Start selected local track |
| `PLAYBACK_STARTED` | AudioService | NavigationService/RenderService | Enter Now Playing and render |
| `PLAYBACK_PAUSED` | AudioService | RenderService | Render paused state |
| `SPOTIFY_CONNECT_REQUESTED` | InputService/Navigation | AudioService/ConnectivityService | Prepare Spotify Connect session |
| `SPOTIFY_CONNECTION_LOST` | InputService/Spotify service | AudioService/ConnectivityService | Stop playback, show popup |
| `WIFI_CONNECTION_LOST` | WiFi service | AudioService/ConnectivityService | Stop playback, show popup |
| `BLUETOOTH_CONNECTION_LOST` | BluetoothService | ConnectivityService | Mark reconnecting |
| `LOW_BATTERY_SIMULATED` | InputService/tests | PowerService | Enter critical power budget |
| `CYCLE_THEME_REQUESTED` | InputService/Settings | ThemeService | Update active theme |
| `RENDER_REQUESTED` | Any state owner | RenderService | Present latest AppState |
| `SHUTDOWN_REQUESTED` | InputService/PowerService | PowerService | Enter shutdown |

## Spotify Loss Contract

```text
SPOTIFY_CONNECTION_LOST or WIFI_CONNECTION_LOST
-> AudioService stops playback
-> ConnectivityService posts blocking popup
-> OK
-> NavigationService opens Local Library
```

