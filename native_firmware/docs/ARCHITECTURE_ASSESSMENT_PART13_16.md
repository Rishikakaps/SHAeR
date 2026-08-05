# Architecture Assessment: Parts 13-16

## Current Boundaries

- Application behavior lives in `firmware/app` and `firmware/core`.
- Storage lives in `firmware/storage`; Marginalia persistence is isolated in `NotebookStore`.
- Theme metadata, registration, assets, transitions, and blueprints live in `firmware/theme`.
- Rendering consumes `RenderModel` and emits `UiCommand` values through `firmware/ui`.
- Hardware access remains behind HAL interfaces.
- Companion and background services are separate from the display renderer.

## Implemented In This Pass

- Theme package registration now validates SDK V1 manifest completeness before accepting a definition.
- `ScreenManager::registrations()` is the single screen metadata registry.
- SDK compatibility and ownership rules are documented in `docs/SDK_V1.md`.
- The host validation gate covers theme, screen, UI, navigation, storage, simulator, and companion regressions.

## Known Gaps

- The on-disk theme packages currently use `theme.json` plus `theme.properties`; the loader still consumes the properties fallback rather than a complete `manifest.json` parser.
- Several screen behaviors are still placeholders or reserved states, including recorder, sync, storage maintenance, and full error recovery.
- The native Pi HAL has no stylus event source yet. `ShaerApp::record_stroke()` is the application boundary for the future input driver.
- Full package asset validation, third-party widget registration, layout registration, and optional-service SDK hooks are planned.
- The host gate is not Raspberry Pi software validation, and neither is physical hardware acceptance.

## Required Next Validation

1. Add real manifest parsing and validate each package's required assets.
2. Replace demo library fixtures with the live music-store view model.
3. Add recorder and stylus HAL contracts before claiming those acceptance tests.
4. Run the Pi software gate and then target-device validation when the Pi is reachable.
