# SHAeR Dependency Graph V1

Status: Required before hardware testing.

```mermaid
flowchart TD
  Input["Input HAL"] --> EventBus["EventBus"]
  EventBus --> Navigation["NavigationService"]
  EventBus --> Audio["AudioService"]
  EventBus --> Connectivity["ConnectivityService"]
  EventBus --> Power["PowerService"]
  EventBus --> ThemeService["ThemeService"]
  Navigation --> AppState["AppStateStore"]
  Audio --> AudioHAL["AudioOutput HAL"]
  Connectivity --> AppState
  Power --> AppState
  ThemeService --> AppState
  AppState --> ScreenManager["ScreenManager"]
  ScreenManager --> ThemeEngine["ThemeEngine"]
  ThemeEngine --> ThemePacks["Theme Packs"]
  ScreenManager --> RenderModel["RenderModel"]
  RenderModel --> RenderService["RenderService"]
  RenderService --> DisplayHAL["Display HAL"]
  SettingsDB["settings.db"] --> BootFirmware["BootFirmware"]
  BootFirmware --> CrashRecovery["BootRecoveryManager"]
  CrashRecovery --> SettingsDB
  LocalFiles["Music Folder"] --> LocalLibrary["LocalLibraryService"]
  LocalLibrary --> AppState
```

## Dependency Rule

HALs may depend on Linux APIs. Firmware services may depend on HAL interfaces. UI/rendering may depend on AppState snapshots. Themes may never depend on services.

