# SHAeR Layer 11 Backend

This backend contains the Layer 11 library foundation and the local Layer 12
Spotify implementation: OAuth 2.0 PKCE, private persistent tokens, automatic
refresh, centralized Web API calls, source-neutral playback state, metadata and
artwork caches, unified-library fields, and a librespot service adapter.

The firmware redesign adds source-neutral media provider contracts in
`shaer_music/providers.py` and Raspberry Pi driver boundaries in `shaer_hal/`.
Theme code does not import either backend or hardware implementation details.

It deliberately does not redesign the theme UI. Spotify and local content are
represented as backend metadata for the unified library.

Run tests from this folder:

```bash
PYTHONPATH=. python3 -m unittest discover -s tests
```

Run diagnostics:

```bash
PYTHONPATH=. python3 diagnostics/run_diagnostics.py
```

Physical completion still requires a credentialed Spotify login, a Raspberry Pi
`librespot` build, PCM5102 ALSA verification, and the hardware acceptance test
in `../shaer_pi_os/LAYER12_SPOTIFY.md`.
