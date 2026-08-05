# SHAeR Theme Validation Harness

The harness renders the same thirteen-state sequence for every selectable SHAeR theme:

`Boot -> Home -> Library -> Album -> Now Playing -> Recording -> Volume -> Queue -> Bluetooth -> Spotify Login -> Loading -> Error -> Shutdown`

It validates:

- stable 3:4 device geometry
- controls, text, artwork, and popups remaining inside the screen
- horizontal and vertical text clipping
- popup-control overlap
- required Now Playing bindings
- queue content
- Local/Spotify geometry parity
- browser console errors
- one shared firmware core mounted for every theme
- deterministic Home, Library, Album-capability, Now Playing, and Back transitions
- exactly one encoder-selected control per navigable page
- rejection of playing-without-a-track and playback-while-recording states
- capability-gated Settings IA, including About first, no Recorder top-level row, and hardware-enabled Power only
- optional deterministic screenshot baselines

Create or intentionally refresh baselines:

```bash
node outputs/theme_validation/theme-validation.mjs --update-baselines
```

Run the regression gate:

```bash
node outputs/theme_validation/theme-validation.mjs
```

Run layout and screenshot validation without an HTTP server:

```bash
node outputs/theme_validation/theme-validation.mjs --static --no-baseline
```

Static mode exercises theme files and the shared bridge directly. It does not replace API integration tests.

Use an already-running device or Pi server:

```bash
node outputs/theme_validation/theme-validation.mjs --base-url http://shaer.local:8775
```

Screenshots and `report.json` are written to `outputs/theme_validation/artifacts/`.
Baseline changes must be reviewed visually before using `--update-baselines`.
