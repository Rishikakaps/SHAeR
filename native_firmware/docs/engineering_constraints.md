# SHAeR Engineering Constraints

These constraints guide implementation while the real hardware is still evolving.

## Navigation

- One C++ process owns UI, input, playback state, notifications, and theme rendering.
- `PlaybackSnapshot` in the app layer is the only source of playback truth.
- Audio HAL implementations are actuators only; they must not become state authorities for UI or navigation.
- Navigation uses a bounded stack with a maximum depth of 8.
- Firmware state is separate from navigation stack.
- Blocking popups are modal and do not push ordinary screens onto the stack.
- Recovery flows must be explicit: Spotify loss stops playback, asks for OK, then opens Local Library.
- Every screen change must emit a transition plan: from screen, to screen, style, duration, input blocking, and reason.
- Back navigation must never reveal stale interrupted playback after a recovery handoff.

## Connections

- Connection state is separate from UI copy.
- Spotify playback uses `SpotifyActive`.
- WiFi or Connect loss moves through `SpotifyRecovering`, then `LocalOffline` after OK.
- Bluetooth output loss moves through `BluetoothRecovering` and returns to the previous screen after OK.
- Connection recovery popups are modal but gentle; the user gets exactly one clear next action.

## Rendering

- UI code must target `RendererAPI v1`, not SDL, framebuffer, or terminal output directly.
- Renderer implementations remain in-process.
- `ConsoleRenderer` is the current simulator renderer.
- Future `SDLRenderer` and `PiRenderer` should implement the same primitive API without changing navigation, theme, or power state logic.

## Theme Worlds

Each theme must have its own:

- layout signature
- transition signature
- animation vocabulary
- emotional tone
- screen blueprint for Home
- screen blueprint for Library
- screen blueprint for Now Playing
- screen blueprint for Settings
- screen blueprint for Popup

No theme should be a palette swap of another theme. Shared C++ renderer primitives are allowed, but each theme profile must compose them into a different experience.

The browser simulator in `simulator/web/index.html` is the current visual review surface. It should stay ahead of the hardware renderer: before a theme structure lands in SDL2/LVGL, it should be visible there and covered by `tests/web_simulator_checks.py`.

ThemeEngine must not know why power changes happen. It exposes preferred motion and transition values; the app-level power policy applies battery saver, critical battery, or quality-mode reductions afterward.

Indian Raga palette: deep indigo background, lavender luminous layer, burnt sienna highlights. Do not revert it to red/yellow/gold.

## Battery-Aware Animation

Themes are allowed to be rich, but the animation policy must adapt:

- normal state: theme-specific rich animation budget
- low battery: reduced FPS and fewer animated elements
- recovery or power-sensitive screens: calmer transitions
- low battery: transition duration capped at reduced-motion timing
- battery saver: WiFi power-save enabled, Bluetooth idle disabled, animation budget reduced
- archive quality: animation budget slightly reduced to reserve CPU for audio

The device should feel alive without wasting runtime on motion the user is not actively benefiting from.

Battery saver must remain visible in simulation, not just hidden in code. A reviewer should be able to see reduced motion, lower battery state, and calmer connection handoffs without plugging in hardware.

## Frozen Battery Architecture

V1 hardware assumes one removable 18650 Li-Ion cell in a plastic single-cell holder. The holder mounts to the enclosure floor or rear shell structure, not the perfboard. The battery can be removed after opening only the rear shell, without desoldering and without disturbing the Pi, DAC, display, controls, or perfboard.

All wiring, CAD, power, assembly, implementation, and service docs must preserve:

- 18650 holder to IP5306 BAT+/BAT-.
- IP5306 5 V rail to Raspberry Pi Zero 2 W.
- dedicated battery compartment.
- cable routing channel from holder to IP5306.
- clearance around holder spring contacts.
- short, quiet DAC analog path away from power wiring.

## Upgradability and Compactness

The app layer must never depend directly on replaceable hardware details.

Keep behind HAL interfaces:

- display panel and driver
- DAC and audio output path
- encoder/button input
- battery fuel gauge
- Bluetooth state

Physical targets the code should preserve:

- display replaceable without rewriting UI logic
- DAC replaceable without rewriting playback state logic
- battery fuel gauge replaceable without changing power behavior
- controls replaceable without changing navigation code
- compact layout protected by short, explicit HAL boundaries instead of scattered hardware calls
