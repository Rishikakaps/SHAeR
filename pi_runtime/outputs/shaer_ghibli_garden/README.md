# SHAeR Ghibli Garden

Standalone 240x320 Ghibli garden theme with supplied background images and liquid-glass controls.

Reference art is normalized into `assets/backgrounds/*.png`; the UI layer stays live for menu selection, playback, voice memos, settings, and encoder navigation.

Run through the Pi OS server:

```bash
python3 ../shaer_pi_os/server.py --theme shaer_ghibli_garden --gpio
```

Open:

```text
http://127.0.0.1:8775/shaer_ghibli_garden/?mode=device
```
