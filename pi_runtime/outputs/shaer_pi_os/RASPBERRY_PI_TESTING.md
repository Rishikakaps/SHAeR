# SHAeR Pi OS Raspberry Pi Test Guide

This build expects:

- Rotary encoder A / CLK on BCM GPIO 17
- Rotary encoder B / DT on BCM GPIO 27
- Separate OK push button on BCM GPIO 22
- Separate Back push button on BCM GPIO 23
- No encoder click switch required

Each button should connect between its GPIO pin and GND. The server uses internal pull-ups.

## Install Pi Dependencies

```bash
sudo apt update
sudo apt install -y python3-gpiozero python3-lgpio chromium-browser
```

If your Raspberry Pi OS image uses `chromium` instead of `chromium-browser`, use `chromium` in the kiosk commands below.

## Unpack The Bundle On The Pi

```bash
mkdir -p ~/shaer
tar -xzf shaer_pi_os_bundle.tar.gz -C ~/shaer
cd ~/shaer/outputs
chmod +x shaer_pi_os/start_theme.sh
```

## Start One Theme With Hardware

```bash
./shaer_pi_os/start_theme.sh shaer_dark_archive
```

Open the UI:

```bash
chromium-browser --kiosk http://127.0.0.1:8775/shaer_dark_archive/?mode=device
```

## Custom GPIO Pins

```bash
PIN_A=17 PIN_B=27 PIN_OK=22 PIN_BACK=23 ./shaer_pi_os/start_theme.sh shaer_dark_archive
```

## Test Without Hardware

Terminal 1:

```bash
python3 shaer_pi_os/server.py --host 127.0.0.1 --port 8775 --theme shaer_dark_archive --allow-test-input
```

Terminal 2:

```bash
curl -X POST -H 'Content-Type: application/json' -d '{"action":"right"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"left"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"select"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"back"}' http://127.0.0.1:8775/api/debug/input
```

To simulate Volume Mode, send two OK/select events within 400 ms:

```bash
curl -X POST -H 'Content-Type: application/json' -d '{"action":"select"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"select"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"right"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"left"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"back"}' http://127.0.0.1:8775/api/debug/input
```

## Theme Test Commands

Run one theme at a time. Stop the current server with `Ctrl+C`, then run the next command.

```bash
./shaer_pi_os/start_theme.sh shaer_dark_archive
chromium-browser --kiosk http://127.0.0.1:8775/shaer_dark_archive/?mode=device
```

```bash
./shaer_pi_os/start_theme.sh shaer_bombay_ticket
chromium-browser --kiosk http://127.0.0.1:8775/shaer_bombay_ticket/?mode=device
```

```bash
./shaer_pi_os/start_theme.sh shaer_japanese_punk
chromium-browser --kiosk http://127.0.0.1:8775/shaer_japanese_punk/?mode=device
```

```bash
./shaer_pi_os/start_theme.sh shaer_windows_xp
chromium-browser --kiosk http://127.0.0.1:8775/shaer_windows_xp/?mode=device
```

```bash
./shaer_pi_os/start_theme.sh shaer_ghibli_garden
chromium-browser --kiosk http://127.0.0.1:8775/shaer_ghibli_garden/?mode=device
```

```bash
./shaer_pi_os/start_theme.sh shaer_indian_print
chromium-browser --kiosk http://127.0.0.1:8775/shaer_indian_print/?mode=device
```

## Navigation Test For Each Theme

1. Rotate clockwise three clicks. Selection should move down/right one item per detent.
2. Rotate counter-clockwise three clicks. Selection should move up/left one item per detent.
3. Press OK. It should open the selected item or toggle the selected control.
4. Press Back. It should return to the previous page.
5. Enter a song/list/settings page and rotate past visible rows. The selected row should scroll into view.
6. Open now playing and press OK on play/pause. The icon/state should toggle.
7. Open voice memos and press OK on record. The recording state/time or waveform should respond.

## Dual-Mode Encoder Test

1. Reboot or refresh the kiosk page. The encoder should start in Navigation Mode.
2. Rotate clockwise. The selection should move down/right, not change volume.
3. Press OK twice within about 400 ms. The volume overlay and `VOL` badge should appear.
4. Rotate clockwise. Volume should increase by 3%, clamped at 100%.
5. Rotate counter-clockwise. Volume should decrease by 3%, clamped at 0%.
6. Press OK once. Volume Mode should exit and the encoder should return to Navigation Mode.
7. Enter Volume Mode again, then press Back. It should exit immediately.
8. Enter Volume Mode again and do nothing for 3 seconds. It should exit automatically.
9. Enter Volume Mode again, then open another screen or play another song. It should exit automatically.

## Fullscreen Exit

Press `Alt+F4` or `Ctrl+W` with a keyboard attached, or reboot from SSH:

```bash
sudo reboot
```
