# SHAeR Thread Ownership V1

Status: Required before hardware testing.

## Firmware V1

V1 firmware runs one deterministic foreground scheduler. Threads are added only when blocking hardware or filesystem work proves it is necessary.

| Owner | Thread | Owns | May Not Own |
|---|---|---|---|
| MainScheduler | Main thread | Event dispatch, AppState mutation, service update cadence | Blocking network calls |
| RenderService | Main thread in V1 | RenderModel generation and display HAL calls | Business logic |
| AudioService | Main thread command owner; decoder may fork/process later | Playback state and ALSA command handoff | UI state |
| LocalLibraryService | Main thread in V1; future worker allowed | Library scan requests and results | Renderer |
| Python Spotify service | Separate process | OAuth, metadata, token refresh | Foreground UI |
| Companion app | Separate desktop process | Sync, imports, firmware staging | Device runtime state |

## Future Thread Rule

Any new thread must document:

| Required Field | Meaning |
|---|---|
| Owner service | The only service allowed to mutate its internal data |
| Input events | Events it consumes |
| Output events | Events it publishes |
| Shared data | Must be immutable snapshot or queued message |
| Shutdown path | How it stops before Linux shutdown |

