# SHAeR Pi OS Runtime

This folder makes the rendered themes usable on a Raspberry Pi with a rotary encoder and push buttons.

## Controls

- Rotary clockwise: move selection down/right
- Rotary counter-clockwise: move selection up/left
- OK push button: select current item
- Back push button: return to the previous page
- Optional home button: return toward home

Default GPIO pins use BCM numbering:

- Encoder A / CLK: GPIO 17
- Encoder B / DT: GPIO 27
- OK push button: GPIO 22
- Back push button: GPIO 23
- Optional home button: disabled unless `--pin-home` is set

## Run On Raspberry Pi

Copy `shaer_pi_os_bundle.tar.gz` to the Pi, then unpack it:

```bash
mkdir -p ~/shaer
tar -xzf shaer_pi_os_bundle.tar.gz -C ~/shaer
cd ~/shaer/outputs
```

Build the archive from the repository root with `bash build_pi_bundle.sh`. The builder excludes AppleDouble files, `.DS_Store`, caches, validation artifacts, and pending restore files, then prints the SHA-256 checksum.

Start the default Dark Archive theme:

```bash
python3 shaer_pi_os/server.py --host 0.0.0.0 --port 8775 --theme shaer_dark_archive --gpio
```

The rotary encoder does not need a click switch. Use two separate momentary push buttons:

```bash
python3 shaer_pi_os/server.py --host 0.0.0.0 --port 8775 --theme shaer_dark_archive --gpio --pin-a 17 --pin-b 27 --pin-ok 22 --pin-back 23
```

Open Chromium in kiosk mode:

```bash
chromium-browser --kiosk http://127.0.0.1:8775/shaer_dark_archive/?mode=device
```

Swap themes by changing `--theme` and the URL:

- `shaer_base_dark`
- `shaer_base_light`
- `shaer_dark_archive`
- `shaer_bombay_ticket`
- `shaer_japanese_punk`
- `shaer_windows_xp`
- `shaer_ghibli_garden`
- `shaer_indian_print`

Or use the launcher:

```bash
./shaer_pi_os/start_theme.sh shaer_dark_archive
PIN_OK=5 PIN_BACK=6 ./shaer_pi_os/start_theme.sh shaer_indian_print
```

## Test Without GPIO

Run the server without `--gpio` and explicitly enable the loopback-only bench input endpoint:

```bash
python3 shaer_pi_os/server.py --host 127.0.0.1 --port 8775 --theme shaer_dark_archive --allow-test-input
```

Then trigger inputs from another terminal:

```bash
curl -X POST -H 'Content-Type: application/json' -d '{"action":"right"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"select"}' http://127.0.0.1:8775/api/debug/input
curl -X POST -H 'Content-Type: application/json' -d '{"action":"back"}' http://127.0.0.1:8775/api/debug/input
```

The browser bridge converts those events into the same keyboard controls the themes already use. This endpoint is disabled unless `--allow-test-input` is supplied and rejects non-loopback clients.

## Layer 11 Backend Diagnostics

The bundle also includes the first Layer 11 backend slice in `shaer_backend`.
From `~/shaer/outputs`, run:

```bash
cd shaer_backend
PYTHONPATH=. python3 -m unittest discover -s tests
PYTHONPATH=. python3 diagnostics/run_diagnostics.py
```

These checks cover the SQLite schema, library indexing fallback, playback queue
snapshot, listening statistics, and Spotify/local matching confidence logic.

## Layer 15 Voice Recording

Install the production GStreamer/ALSA capture runtime and recording service:

```bash
cd ~/shaer/outputs
chmod +x shaer_pi_os/install_layer15.sh
./shaer_pi_os/install_layer15.sh
```

Confirm the microphone appears in ALSA, then run the hardware diagnostic mode:

```bash
arecord -l
cd ~/shaer/outputs/shaer_backend
SHAER_HARDWARE=1 PYTHONPATH=. python3 diagnostics/microphone_test.py
PYTHONPATH=. python3 diagnostics/recording_test.py
PYTHONPATH=. python3 diagnostics/playback_test.py
```

Recorder controls are:

- Short OK: start, pause, or resume.
- Long OK (0.8 seconds): finish and save.
- Back during recording: Keep/Discard confirmation.
- Rotate and OK in Recordings: browse and play from the personal archive.

Layer 15 design and recovery details are in `../docs/VOICE_RECORDING_ARCHITECTURE.md` and the other `RECORDING_*.md` files.

Layer 12 Spotify setup and hardware acceptance steps are in
`LAYER12_SPOTIFY.md`. Deploy a rebuilt bundle with:

```bash
./shaer_pi_os/deploy_to_pi.sh <pi-host-or-ip> <pi-user>
```

The installed system service enables the Power setting. It remains protected
by a one-use physical OK-button capability and the narrowly scoped
`shaer-power-sudoers` rule. Bench servers omit `--allow-power`, so Power is
hidden rather than presenting a setting that cannot act.

The shared runtime, state contract, HAL/provider boundaries, and remaining
physical acceptance gates are documented in
`../docs/FIRMWARE_CORE_ARCHITECTURE.md`.

## Optional System Service

Edit `shaer_pi_os/shaer-pi-os.service` if your Pi user/path is not `/home/pi/shaer/outputs`, then install it:

```bash
sudo cp shaer_pi_os/shaer-pi-os.service /etc/systemd/system/shaer-pi-os.service
sudo systemctl daemon-reload
sudo systemctl enable --now shaer-pi-os.service
```
