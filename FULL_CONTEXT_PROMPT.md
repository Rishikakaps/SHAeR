# SHAeR Full Context and Continuation Prompt

You are continuing work on SHAeR, a custom Raspberry Pi Zero 2 W music player, Spotify device, local-music player, voice recorder, personal archive, and companion-device ecosystem.

This is an existing product build. Do not restart it, create a third implementation, or substitute a generic UI. Inspect the current files before editing and preserve working behavior.

## 1. Canonical folder and source of truth

The one canonical combined codebase is:

```text
/Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
```

Start every future task here:

```bash
cd /Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
```

Do not continue work independently in the old dated folders. They are provenance snapshots, not active branches.

The unified folder is not itself a Git checkout. Do not assume `git status` is available there.

Read these files first:

1. `FULL_CONTEXT_PROMPT.md`
2. `NEXT_CHAT_START_HERE.md`
3. `INTEGRATION_MAP.md`
4. `PROVENANCE.md`
5. `pi_runtime/outputs/docs/FIRMWARE_CORE_ARCHITECTURE.md`
6. `native_firmware/docs/SYSTEM_ARCHITECTURE.md`
7. `native_firmware/docs/STATE_MACHINE.md`

## 2. Product vision and non-negotiable design intent

SHAeR is meant to behave like a small, polished consumer music device, not a web demo running on a Raspberry Pi.

The primary physical controls are:

- rotary encoder rotation for previous/next focus movement; encoder click is not required
- one physical OK/select button
- one physical Back button that always returns to the previous page or a safe Home fallback

The device should eventually be usable without a terminal, keyboard, or SSH after installation.

The six themes are independent UI worlds, not one shared layout with six color palettes:

1. Archive Dark
2. Bombay Local / Bombay Ticket
3. Japanese Punk
4. Windows XP
5. Ghibli Garden
6. Indian Raga / Indian Print

Preserve their specific typography, layouts, borders, artwork, backgrounds, loading screens, charging screens, menus, controls, and animation language. Shared behavior should live below the renderers, while the renderers remain visually distinct.

The target display geometry is 240 x 320, a stable 3:4 ratio. Encoder focus must be visible, selectable elements must not overlap, and text/artwork must stay inside the display.

Never reintroduce production demo data such as `SONG 1`, `ARTIST NAME`, `PLAYLIST 1`, fake album names, fixed sample queue rows, or fake library counts. Diagnostic fixtures are allowed only behind explicit validation query parameters.

Every data view must support truthful states:

```text
LOADING
REAL DATA
EMPTY
ERROR
OFFLINE
UNAUTHENTICATED
UNAVAILABLE CAPABILITY
```

Do not replace errors or empty results with realistic-looking sample data.

## 3. Two implementation tracks in the unified folder

### A. Native foreground firmware

Path:

```text
native_firmware/
```

This is the C++17 long-term foreground runtime. It contains:

- event bus and scheduler
- app state and state machine
- screen and resource managers
- notification and logging foundations
- desktop and Raspberry Pi HAL boundaries
- boot/recovery flow
- SQLite settings and music-library foundations
- audio playback foundation
- native UI/theme framework
- simulator, diagnostics, tests, and systemd/install scripts

The intended final architecture is for native C++ to own foreground lifecycle, page state, focus, Back behavior, overlays, animations, selected media, and hardware input.

### B. Current complete Pi prototype and product services

Path:

```text
pi_runtime/outputs/
```

This is currently the most complete runnable prototype. It contains:

- six high-fidelity browser-rendered device themes
- shared JavaScript firmware/navigation runtime
- Python backend and SQLite music services
- Spotify PKCE authentication and Web API integration
- Librespot management foundation
- queue, playback, saved music, playlists, recent music, and search services
- GPIO input polling and physical confirmation capabilities
- voice recording and personal archive services
- companion protocol, pairing, linked-device management, backup/restore, and security work
- shared browser/Android/Windows-PWA companion frontend
- theme-validation harness
- Raspberry Pi deployment and systemd files

Do not blindly merge the native and Pi-runtime versions of state, settings, databases, GPIO mappings, queues, or service units. Write a contract test, choose one owner, migrate callers, then remove the superseded owner.

## 4. Actual Pi runtime location and service identity

The intended Raspberry Pi user and runtime path are:

```text
user: aditya
runtime: /home/aditya/shaer/outputs
service: shaer-pi-os
local server: http://127.0.0.1:8775
```

Do not introduce `/home/pi/...` paths.

Important current defect: `pi_runtime/outputs/shaer_pi_os/deploy_to_pi.sh` still defaults `PI_USER` to `pi`. Change the default to `aditya` before the next deployment, while still allowing an explicit user argument.

## 5. Theme entry points

Each theme loads its own renderer plus the same shared music store, firmware core, and hardware bridge:

```text
pi_runtime/outputs/shaer_dark_archive/index.html
pi_runtime/outputs/shaer_bombay_ticket/index.html
pi_runtime/outputs/shaer_japanese_punk/index.html
pi_runtime/outputs/shaer_windows_xp/index.html
pi_runtime/outputs/shaer_ghibli_garden/index.html
pi_runtime/outputs/shaer_indian_print/index.html
```

Shared device files:

```text
pi_runtime/outputs/shaer_pi_os/music-store.js
pi_runtime/outputs/shaer_pi_os/firmware-core.js
pi_runtime/outputs/shaer_pi_os/hardware-bridge.js
pi_runtime/outputs/shaer_pi_os/system-overlays.css
pi_runtime/outputs/shaer_pi_os/server.py
pi_runtime/outputs/shaer_pi_os/spotify_api.py
```

The current frontend is framework-free HTML/CSS/JavaScript. Do not add React or another framework to the 240 x 320 device UI without a strong, measured reason.

## 6. Completed product layers before the latest UI repair

The combined work includes the following major layers.

### Local music and unified library

- SQLite music-library and playlist foundations
- local track scanning/indexing and companion endpoints
- source-neutral track concepts shared between local and Spotify layers
- local-only API access for the on-device UI

### Spotify authentication and services

- OAuth with PKCE
- token persistence with restricted file permissions
- token refresh and logout
- authentication status and profile
- search with Spotify-compatible limits
- current playback
- queue
- saved tracks
- playlists
- recently played
- playback-control routing
- correct handling of Spotify success responses with empty `204` bodies
- Spotify error status/message preservation

Librespot is present but intentionally not enabled by default because physical DAC/ALSA output is still unverified.

### Firmware UI and hardware bridge

- one shared transition table and Back-history contract across themes
- encoder/keyboard focus and activation
- OK/select and Back input flow
- GPIO event polling
- one-use physical capability tokens for protected actions
- system overlays for volume, queue, Bluetooth, Spotify login, errors, pairing, and shutdown
- impossible-state rejection such as playing with no track and recording while playback is active
- capability-gated settings

### Theme validation

The harness is at:

```text
pi_runtime/outputs/theme_validation/theme-validation.mjs
```

It renders this sequence for all six themes:

```text
Boot
Home
Library
Album
Now Playing
Recording
Volume
Queue
Bluetooth
Spotify Login
Loading
Error
Shutdown
```

It checks 3:4 geometry, clipping, control overlap, artwork bounds, required bindings, queue content, Local/Spotify geometry parity, console errors, navigation contracts, focus selection, state invariants, and capability-gated settings.

### Companion application

Shared companion source:

```text
pi_runtime/outputs/shaer_companion/
```

Built artifacts currently present:

```text
releases/SHAeR-Companion-0.17.0-debug.apk
releases/shaer-companion-pwa-0.17.0.zip
```

The shared companion work includes local discovery, pairing, physical approval, remembered devices, revocation, playback/device views, browser fallback, Android Capacitor packaging, Android Keystore credential protection, Android NSD discovery, and an installable Windows/browser PWA path.

The APK is debug-signed. It is an acceptance build, not a public release.

### Voice recording and archive

- recording service and status API
- waveform/status integration
- start/pause/stop/cancel controls
- recording metadata and archive browsing
- playback from the personal archive
- interrupted-recording recovery design and tests
- companion recording browsing/transfer contracts

## 7. Latest UI and real-data repair completed on 18 July 2026

The latest work directly addressed placeholder UI content and usability.

### Shared data architecture

Added:

```text
pi_runtime/outputs/shaer_pi_os/music-store.js
```

It now provides one shared on-device state store with normalized models for:

- current playback
- queue
- Spotify saved tracks
- Spotify playlists
- Spotify recently played tracks
- local tracks
- playlist tracks
- search results
- Spotify configuration/authentication status
- loading, ready, empty, offline, unauthenticated, and error states

It uses request cancellation to prevent stale responses from replacing newer state. Playback polling is centralized and uses slower intervals when idle or hidden.

The six themes no longer initialize fake song arrays. They consume normalized state through `firmware-core.js`.

### Real local and Spotify data routes

Added local on-device routes:

```text
GET /api/music/tracks
GET /api/music/playlists
```

Added real Spotify playlist-detail flow:

```text
GET /api/spotify/playlists/{playlist_id}/tracks
```

The backend calls Spotify playlist items, validates playlist IDs, and normalizes wrapped `item` or `track` objects before rendering.

Selecting a playlist now publishes a loading state and then opens that playlist's real tracks. It no longer silently shows the generic saved-track list.

### Placeholder and obsolete-page removal

- removed hard-coded sample song/artist/playlist arrays from all six theme controllers
- removed the Archive Dark `library-view` / folder-choice page
- removed old folder-navigation labels and routes from production theme navigation
- replaced fake library content with truthful loading/empty/offline/unauthenticated messages
- made queue and Now Playing content use real shared state
- made playlist rows and track rows encoder-selectable
- changed fixed theme dates to live dates where the UI is code-rendered
- changed fake 30 percent battery values to `--%` until a real battery API exists

### Archive Dark repair

- rebuilt the loading page to match the reference identity: `LOADING`, `adi-VASI OS`, bordered progress bar, and `PLEASE WAIT.......`
- added a low-cost stepped progress sweep and subtle opacity animation
- retained the two-frame walking charging animation
- retained waveform animation for real recording state
- added reduced-motion behavior
- corrected shuffle, previous, play/pause, next, and repeat hit targets
- made library, album, queue, and metadata render from real state

### Other theme cleanup

- Bombay Ticket, Japanese Punk, Windows XP, Ghibli Garden, and Indian Print now use real tracks/playlists and honest empty states
- playlist selection was wired for all six themes
- Ghibli library was made scrollable
- reduced-motion rules were added for recurring animation surfaces
- Ghibli no longer displays explanatory `No sample tracks are shown` text

## 8. Verification truth as of this handoff

The following targeted checks passed after the latest edits:

```text
Shared companion/device JavaScript tests: 12 passed
Spotify backend tests: 14 passed
Theme validation states: 78 passed
Theme validation errors: 0
Theme validation warnings: 0
```

The latest theme report is:

```text
pi_runtime/outputs/theme_validation/artifacts/report.json
```

The theme-validation server logged harmless `BrokenPipeError` messages when Playwright navigated away from long-poll event requests. The final harness result still passed. This log noise should be caught/suppressed in `send_json` or the event endpoint as a polish task.

Do not claim that the complete project gate passed after the latest changes yet. `./test_all.sh` still needs to be rerun from the unified root after this UI repair.

## 9. Release truth

Current source version:

```text
0.17.0-integration.1
```

Current firmware-core version:

```text
0.16.0
```

Current companion version:

```text
0.17.0
```

Existing release archives in `releases/` were built before the latest 18 July UI/data changes. They are stale relative to source and must be rebuilt after the full gate passes.

Do not send the existing Pi archive to the device as if it contains the latest edits.

## 10. Known incomplete work and risks

### Immediate software completion tasks

1. Run the full host gate with `./test_all.sh`.
2. Add an HTTP integration test for `/api/spotify/playlists/{id}/tracks` and local device music routes.
3. Make local playlists part of the device store; the local playlist endpoint exists, but the current store only merges local tracks with Spotify playlists.
4. Wire local track selection to the real local playback service. Spotify URI selection is wired; local media selection must not stop at a UI-only state change.
5. Add a real battery/status endpoint and update theme state from hardware. Until then, `--%` is the honest value.
6. Audit the Indian Print image assets. Some screenshots contain baked title/date/battery chrome. Borders and permanent patterns may remain images, but changing data must be code-rendered over clean assets.
7. Review settings in every theme. A setting must either change firmware, be clearly read-only, or be hidden. Do not show inert settings.
8. Catch client-disconnect `BrokenPipeError` without noisy tracebacks.
9. Fix `deploy_to_pi.sh` to default to user `aditya`.
10. Rebuild the Pi bundle, unified source archive, companion artifacts if affected, and all SHA-256 files only after tests pass.

### Architecture work still required

The project still has two foreground-runtime tracks:

- native C++ foreground firmware
- browser/JavaScript foreground prototype

The long-term design is:

```text
Native C++ foreground UI
    -> canonical state machine and Back/history
    -> encoder/buttons and native HAL
    -> native theme renderer
    -> narrow IPC/provider boundary
         -> local library/indexer
         -> Spotify/Connect
         -> recording archive
         -> companion/device management
```

The latest shared JavaScript music store reduces per-theme data duplication, but the six theme renderers still contain repeated page logic. Do not force them into one visual template. Extract semantic view models and contracts while preserving distinct renderer composition.

### Physical and reliability gates still open

These are not passed until run on the real Raspberry Pi and attached hardware:

- display acceptance
- encoder rotation acceptance
- OK and Back button acceptance
- DAC and ALSA output
- battery sensing
- charging status
- Wi-Fi recovery
- Bluetooth behavior
- microphone recording
- safe boot/shutdown
- Spotify login on the physical device
- Spotify Connect discovery and audio transfer
- local playback through the real output
- Android companion pairing/reconnect/revocation
- Windows PWA install/reconnect/uninstall
- ten boot loops
- ten shutdown loops
- ten Spotify transfers
- ten encoder stress loops
- ten playlist changes
- ten theme changes
- power loss during history, playlist, recording, restore, and indexing writes
- nearly full SD card
- corrupt MP3, FLAC, artwork, SQLite, and backup files
- 20,000-track library performance
- 72-hour RAM, CPU, and service soak

Do not describe any of these as passed based on mocks, host tests, or screenshots.

## 11. Companion work still required before public release

- install and test the APK on a real Android phone/tablet
- test discovery permissions, physical OK pairing, reboot reconnection, controls, transfer, and revocation
- install the PWA through Edge on a real Windows laptop
- test keyboard navigation, reconnect, uninstall, and credential cleanup
- replace debug Android signing with protected release signing
- produce versioned release notes and compatibility metadata
- use encrypted authenticated local transport before a public consumer release
- do not expose SHAeR directly to the public internet

## 12. Engineering reasoning to preserve

Use these decision rules:

1. Preserve the visual references. Do not redesign the six theme worlds.
2. Put data fetching, normalization, polling, cancellation, errors, and actions in shared layers.
3. Keep theme files focused on rendering and semantic bindings.
4. Never let a theme parse raw Spotify JSON.
5. Never use demo rows as an error or empty-state fallback.
6. Keep animation cheap on Pi Zero 2 W: prefer opacity, transforms, short stepped loops, and small DOM surfaces.
7. Input wins over animation. No transition may delay encoder, OK, or Back handling.
8. Keep layout dimensions stable so selection and dynamic text do not shift the screen.
9. Treat missing hardware as a capability state, not permission to fabricate values.
10. Treat host tests, Pi software tests, and physical acceptance as separate evidence levels.
11. Before merging duplicated systems, define the contract and choose one authority.
12. Make the smallest change that advances the canonical architecture without breaking the working prototype.

## 13. Exact next implementation order

Continue in this order:

1. Inspect the files changed in the 18 July UI/data repair.
2. Run `./test_all.sh` and fix any regressions without weakening tests.
3. Add route-level tests for playlist details and local music access.
4. Complete local playlist and local playback wiring in the shared music store/provider boundary.
5. Clean dynamic chrome out of Indian Print background images while preserving the exact border/pattern art.
6. Add truthful hardware battery/status state.
7. Fix deploy username and BrokenPipe logging.
8. Visually review key screenshots from all six themes, especially loading, library, album, Now Playing, recording, settings, and shutdown.
9. Rebuild release artifacts and checksums.
10. Deploy to the Raspberry Pi as `aditya` and run software acceptance.
11. Perform display/input/audio/battery/microphone acceptance on the physical device.
12. Only after the device UI is stable, continue the native foreground convergence described in `INTEGRATION_MAP.md`.

## 14. Useful commands

Full host validation:

```bash
cd /Users/rishika/Documents/Codex/2026-07-04/co/shaer_unified
./test_all.sh
```

Build everything after the tests pass:

```bash
./build_all.sh
```

Run the current prototype server:

```bash
python3 pi_runtime/outputs/shaer_pi_os/server.py \
  --host 127.0.0.1 \
  --port 8775 \
  --theme shaer_dark_archive \
  --allow-test-input
```

Open:

```text
http://127.0.0.1:8775/shaer_dark_archive/?mode=device
```

Run theme validation:

```bash
node pi_runtime/outputs/theme_validation/theme-validation.mjs --no-baseline
```

Build the Pi bundle:

```bash
./pi_runtime/build_pi_bundle.sh
```

After fixing the deploy default, deploy over Wi-Fi with an explicit user:

```bash
./pi_runtime/outputs/shaer_pi_os/deploy_to_pi.sh <PI_HOST_OR_IP> aditya
```

## 15. Definition of done

SHAeR is not complete merely because all screens render.

Completion requires:

- one authoritative foreground runtime
- truthful Local and Spotify playback
- real library, queue, playlists, search, artwork, recording, and device state
- six visually faithful and fully navigable themes
- predictable encoder, OK, and Back behavior
- only functional/read-only settings visible
- safe recovery from network, storage, database, media, and hardware failures
- reproducible Pi, Android, Windows/PWA, and browser builds
- no production placeholders
- no terminal required after installation
- full host tests
- Pi software acceptance
- real hardware acceptance
- reliability and soak tests
- installable companion builds tested on physical Android and Windows devices

At every stage, report what was implemented, what was automatically verified, what was manually inspected, and what remains blocked on physical hardware. Never blur those categories.
