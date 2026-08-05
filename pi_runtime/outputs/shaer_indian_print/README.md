# SHAeR Indian Print

Standalone 240x320 Indian print theme with the supplied textile/arch image layers and live controls.

This intentionally skips the empty folder-navigation page from the reference set.

Actual reference art is normalized into `assets/screens/*.png`; the latest Indian screens use the supplied full-panel images, while the UI layer stays live for menu selection, playback, memos, settings, and encoder navigation.

Run through the Pi OS server:

```bash
python3 ../shaer_pi_os/server.py --theme shaer_indian_print --gpio
```

Open:

```text
http://127.0.0.1:8775/shaer_indian_print/?mode=device
```
