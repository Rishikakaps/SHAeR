# आदि Vasi OS / SHAeR System Architecture V1

Status: V1 freeze draft. This file is the project constitution. Any change that breaks this document must be treated as a V2 architecture change, not an incidental edit.

## Product Intent

`आदि Vasi OS` is the embedded operating environment for SHAeR. SHAeR is a retro-futuristic personal audio archive. It plays local music, Spotify Connect sessions, Bluetooth headphones, and voice memories from one handheld object that should feel carefully engineered, repairable, and emotionally warm.

Linux exists underneath as Raspberry Pi OS Lite, but it must remain invisible. The user experience is `आदि Vasi OS`, not Linux.

The target hardware is Raspberry Pi Zero 2 W with a replaceable display, DAC, microphone, battery, encoder, buttons, and power subsystem.

V1 battery hardware is frozen as a removable single 18650 Li-Ion cell in a plastic holder mounted to the enclosure floor/rear shell structure. The holder feeds BAT+ and BAT- on the IP5306 charging/boost module.

Boot target: 6 to 8 seconds from power button to usable Home screen.

## User-Visible OS Rule

From power applied to shutdown complete:

- No boot console.
- No kernel messages.
- No login prompt.
- No desktop.
- No taskbar.
- No mouse cursor.
- No Raspberry Pi splash.
- Every visible pixel belongs to SHAeR.

## Architecture Diagram

```text
SHAeR Device
|
+-- C++ आदि Vasi OS Foreground Process
|   |
|   +-- Main Loop
|   |   +-- Input Manager
|   |   +-- State Machine
|   |   +-- Playback Queue Manager
|   |   +-- Theme Engine
|   |   +-- UI Layer
|   |   +-- RendererAPI v1
|   |   +-- Power Policy
|   |   +-- Notification Manager
|   |   +-- Recovery Console
|   |
|   +-- HAL v1
|       +-- Display
|       +-- AudioOutput
|       +-- MicrophoneInput
|       +-- Battery
|       +-- Bluetooth
|       +-- WiFi
|       +-- Storage
|       +-- PowerButton
|
+-- Python External Services
|   +-- Spotify Metadata/OAuth Service
|   +-- Sync Engine
|   +-- Companion App API
|   +-- Cover Art Cache Worker
|
+-- Linux
    +-- ALSA/PipeWire
    +-- BlueZ
    +-- Wi-Fi stack
    +-- systemd
    +-- filesystem
    +-- SQLite
```

## Language Split

C++ owns all latency-sensitive device behavior:

- UI, navigation, and animation timing.
- Display rendering through RendererAPI v1.
- Button and encoder input.
- Playback state and queue control.
- Local playback control through ALSA.
- Voice recording control.
- Battery saver, AOD, sleep, shutdown, and recovery states.

Python owns integration-heavy external services:

- Spotify OAuth and metadata fetches.
- Token refresh and re-authentication handoff.
- Companion app backend.
- Local library sync, cover art processing, and metadata imports.
- Database migrations and export tools.

Python services never own the foreground UI state. They report status and data to the C++ firmware over one local service interface.

## Event-Driven OS Core

SHAeR V1 uses a central in-process EventBus inside the C++ firmware. Input, navigation, playback, power, connectivity, theme, notification, and render services communicate through events instead of direct service-to-service calls.

The firmware still behaves like a small game engine: one deterministic main scheduler owns ticks, event dispatch, state mutation, service updates, and rendering cadence. The EventBus is not an external broker and does not introduce IPC overhead.

Only external Python services cross a process boundary. V1 IPC is local HTTP on `127.0.0.1` with versioned JSON APIs. Do not introduce D-Bus for V1.

## HAL Structure

Every replaceable hardware component must be behind HAL v1:

```text
Display
+-- DesktopDisplay
+-- PiSpiDisplay

Renderer
+-- ConsoleRenderer
+-- SDLRenderer
+-- PiFramebufferRenderer
+-- TestRenderer

AudioOutput
+-- DesktopAudioOutput
+-- AlsaI2SOutput

MicrophoneInput
+-- DesktopMicrophoneInput
+-- I2SMemsMicrophone

Battery
+-- DesktopBattery
+-- Max17048Battery

Input
+-- KeyboardInput
+-- EC11EncoderInput
+-- GpioButtonInput
```

Application code must not import GPIO, SPI, I2C, ALSA, BlueZ, or display-controller details directly.

## Renderer Abstraction

The UI emits draw commands through RendererAPI v1. SDL, Pi framebuffer/SPI, and test renderers must conform to the same interface. The UI may request text, images, album art, vinyl/progress motifs, popups, transitions, and final presentation. It may not know whether it is drawing to HTML, SDL, or the Pi display.

## State Machine

Firmware state is separate from visual screen. State answers "what is the device doing?" Screen answers "what is being drawn?"

Required V1 states:

```text
BOOT
HOME
LOCAL_LIBRARY
SPOTIFY_CONNECT
PLAYBACK
VOICE_RECORDING
MEMORY_MODE
SETTINGS
POPUP
AOD
CHARGING
SLEEP
SHUTDOWN
RECOVERY
```

The full transition contract lives in `STATE_MACHINE.md`.

## Boot Flow

1. Linux boots silently in the background.
2. `shaer-app.service` starts `आदि Vasi OS`.
3. SHAeR claims the foreground/display as early as possible.
4. Libra constellation animation appears.
5. Text appears: `Loading D: Drive...` and `mhm mhm`.
6. Hardware and services initialize behind the animation.
7. Home fades in.
8. Home must be usable within 6 to 8 seconds.

Cold boot always starts at Home with: Spotify Connect, Local Library, Voice Archive. Cold boot never resumes directly into Now Playing.

## Shutdown Flow

Long press power button:

1. Enter `SHUTDOWN`.
2. Stop recording immediately if active and finalize the file.
3. Fade or pause playback safely.
4. Show slow goodbye message.
5. Flush SQLite, settings, logs, and playback history.
6. Ask systemd/Linux to shut down.

No direct power cut is allowed while filesystem writes are pending.

## Sleep, AOD, And Battery Saver

Short press: enter battery saver without leaving the current user context.

Two quick presses: enter sleep. If sleep lasts 45 minutes with no music playing, SHAeR automatically performs the slow shutdown flow.

AOD is display-minimal, low-refresh, theme-aware, and may show time, battery, and playback status. It is not a full UI.

## Spotify Connect Architecture

Spotify credentials and refresh tokens are stored securely by the Python Spotify service. The user should not normally log in again. If Spotify revokes authorization or refresh fails, the companion app guides re-authentication.

If Wi-Fi disconnects mid-song:

1. Playback stops.
2. A playful, non-alarming popup appears.
3. User presses OK.
4. Local Library opens as the recovery root.

## Local Playback Architecture

The C++ Playback Queue Manager owns local queue, current track, shuffle, repeat, progress, resume position, ReplayGain, crossfade, and playback history. It controls ALSA output and accepts data from the metadata database. Supported V1 file types are MP3, FLAC, and WAV.

Gapless playback is required for local album playback.

## Companion App Architecture

The companion app is a Python backend with a simple local UI/API for:

- First-time setup.
- Spotify authentication and re-authentication.
- Syncing local MP3, FLAC, WAV, and voice recordings.
- Editing ReplayGain and crossfade defaults.
- Managing Memory Mode links.
- Exporting logs and diagnostics.
- OTA update staging.

## Database

V1 device storage uses SQLite. PostgreSQL is allowed only for a future desktop/server companion sync target. The Pi should not require PostgreSQL to boot, play, record, or recover.

Metadata must include:

- Play count.
- Last played.
- Favourite.
- Rating.
- Voice notes linked to tracks, albums, or playlists.
- Spotify URI.
- Cover art cache path.

## Logs

All logs live under `logs/`:

- `system.log`
- `spotify.log`
- `audio.log`
- `power.log`
- `bluetooth.log`
- `crash.log`

Logs must rotate and must be exportable from recovery mode.

## Thread Model

V1 default:

- Main/UI thread: state machine, input, rendering, notifications.
- Audio worker thread: decoding, gapless handoff, ALSA buffering.
- Recorder worker thread: microphone capture and encoding.
- Service client worker: local HTTP calls to Python services.
- Background metadata worker: cover art and DB writes at low priority.

The UI thread must never block on Spotify, sync, network, cover art download, or database migrations.

## Service Communication

Use local HTTP JSON on `127.0.0.1` for Python services:

- `/api/v1/spotify/status`
- `/api/v1/spotify/metadata`
- `/api/v1/sync/status`
- `/api/v1/companion/pairing`
- `/api/v1/memory-links`

Every API response includes `api_version`, `ok`, and `error_code`.

## Recovery Philosophy

Errors are playful, clear, and never alarming. Recovery always offers a next safe action. The user should feel the object is composed, not broken.

Recovery modes required in V1:

- Display test.
- Button test.
- Encoder test.
- Audio test.
- Battery status.
- Bluetooth test.
- Wi-Fi test.
- SD card test.
- Factory reset.
- Export logs.
