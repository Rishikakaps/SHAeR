# Development Tracks

SHAeR development is split into two tracks. Track A must stay focused on hardware-ready playback. Track B starts only after the breadboard prototype is stable.

## Track A - Functional Prototype

Goal: make the breadboard behave like a finished local MP3 player as quickly and reliably as possible.

Track A owns:

- Boot into SHAeR automatically.
- Initialize the 2.4" IPS TFT SPI display.
- Show the boot animation correctly.
- Render the Home screen.
- Read the EC11 rotary encoder and buttons.
- Scan MP3 files from the SD card.
- Browse the local music library.
- Play MP3 files through the PCM5102A DAC.
- Adjust volume without crashing.
- Return from playback to the library.
- Shut down safely.

Track A does not own advanced visual polish, final theme artwork, Spotify, voice recording, companion app features, or secret features.

## Track B - UI Polish

Goal: replace the functional prototype visuals with the final product experience after the playback pipeline is proven.

Track B owns:

- Final icons and image assets.
- Archive Dark.
- Bombay Ticket.
- Windows-style theme.
- Indian Raga.
- Ghibli Garden.
- Punk/alternate theme.
- Animations.
- Transitions.
- Visual polish.

Track B must not change playback, storage, audio, shutdown, or hardware validation behavior unless a verified bug requires it.

## Milestone Order

| Milestone | Name | Track | Completion Target |
|---:|---|---|---|
| 1 | Boot, TFT, Navigation, MP3 Playback | A | Complete local playback vertical slice |
| 2 | Hardware Diagnostics And Breadboard Verification | A | Every module has an independent diagnostic |
| 3 | UI Framework | B | Screen structure without theme-specific logic |
| 4 | Theme Engine | B | Data-driven themes without business logic |
| 5 | Archive Dark | B | Theme complete across every screen |
| 6 | Bombay Ticket | B | Theme complete across every screen |
| 7 | Windows-style Theme | B | Theme complete across every screen |
| 8 | Indian Raga | B | Theme complete across every screen |
| 9 | Ghibli Garden | B | Theme complete across every screen |
| 10 | Punk/Alternate Theme | B | Theme complete across every screen |
| 11 | Spotify | A | Spotify Connect integrated without UI coupling |
| 12 | Voice Recorder | A | Voice memories recorded and indexed |
| 13 | Companion App | A | Desktop management workflow usable |
| 14 | Notifications | A | System popups and recovery notices stable |
| 15 | Secret Features | B | Hidden interactions after core stability |
| 16 | Performance Optimization | A | Measured CPU, RAM, boot, and power budgets |
| 17 | Perfboard Readiness | A | Breadboard learnings translated to solder plan |
| 18 | Final Enclosure Validation | A | Hardware, wiring, thermal, and service access validated |

## Track A Milestone 1 Gate

Milestone 1 is complete only after the following sequence passes on the Raspberry Pi breadboard setup:

1. Power on the Raspberry Pi.
2. See the 2.4" IPS TFT SPI display initialize.
3. Watch the Libra boot animation.
4. Navigate using the EC11 rotary encoder.
5. Browse MP3 files from the SD card.
6. Start playback through the PCM5102A DAC.
7. Adjust volume.
8. Return to the library.
9. Shut down safely.
10. Repeat the full process for 10 consecutive boot cycles with no crashes and no manual intervention beyond the intended controls.

## Display Target

The V1 prototype display target is the 2.4" IPS TFT SPI display, treated in code as an ILI9341-style 240x320 controller until hardware validation proves otherwise.

Do not describe the V1 display target as OLED.
