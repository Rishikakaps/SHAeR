# Breadboard Prototype Validation

Layer: Foundation to first local-playback prototype.

This procedure validates the minimal runnable SHAeR pipeline before perfboard soldering:

```text
Power on
-> TFT display initialization
-> boot animation
-> home screen
-> encoder navigation
-> SD-card music scan
-> MP3 playback through PCM5102A
-> volume control
-> safe shutdown
```

## Required Components

| Component | Notes |
|---|---|
| Raspberry Pi Zero 2 W | Raspberry Pi OS Lite, SSH enabled |
| SPI display wired as V1 display | ILI9341-style 240x320 currently targeted |
| EC11 rotary encoder | A/B/push wired to GPIO table |
| Back, Play/Pause, Options buttons | Pull GPIO to ground when pressed |
| PCM5102A DAC | I2S output, `dtoverlay=hifiberry-dac` |
| microSD card | Contains OS, repository, and `/var/lib/shaer/music` |
| Stable 5 V power | Use bench/USB power before battery boost path |

## GPIO Pins

Use `docs/GPIO_MASTER_TABLE.md` as the only pin source.

| Function | GPIO |
|---|---:|
| Encoder A | GPIO5 |
| Encoder B | GPIO6 |
| Encoder push | GPIO7 |
| Back | GPIO12 |
| Play/Pause | GPIO13 |
| Options | GPIO16 |
| Power button | GPIO4 |
| Display MOSI | GPIO10 |
| Display SCLK | GPIO11 |
| Display CS | GPIO8 |
| Display DC | GPIO25 |
| Display Reset | GPIO24 |
| Display Backlight | GPIO26 |
| DAC BCLK | GPIO18 |
| DAC LRCLK | GPIO19 |
| DAC DIN | GPIO21 |

## Pi Configuration

`/boot/firmware/config.txt` should include:

```text
dtparam=spi=on
dtparam=i2c_arm=on
dtparam=audio=off
dtoverlay=hifiberry-dac
```

Reboot after changing overlays.

## Build

```bash
cd /home/tuku/SHAeR
make pi
```

Expected:

- `build/shaer_pi_bringup`
- `build/shaer_diag`
- zero compiler errors

## Independent Diagnostics

Run each diagnostic before running the full app:

```bash
sudo ./build/shaer_diag display
sudo ./build/shaer_diag gpio
sudo ./build/shaer_diag encoder
sudo ./build/shaer_diag audio
sudo ./build/shaer_diag sdcard
sudo ./build/shaer_diag battery
./build/shaer_diag wifi
```

Expected:

- 2.4" IPS TFT SPI display shows boot/home/library/now-playing text screens.
- GPIO released buttons read `1`; pressed buttons read `0`.
- Encoder rotation prints action numbers.
- Audio test plays a 440 Hz tone through PCM5102A.
- SD-card test prints `/var/lib/shaer/music` and MP3 count.
- Battery test prints fuel-gauge percentage or fallback.
- Wi-Fi test reports link status or stores details in `/tmp/shaer_wifi_test.log`.

## Full App Runtime

Copy MP3 files:

```bash
sudo mkdir -p /var/lib/shaer/music
sudo cp *.mp3 /var/lib/shaer/music/
sudo chown -R shaer:shaer /var/lib/shaer/music
```

Run:

```bash
sudo ./build/shaer_pi_bringup
```

Expected behavior:

- 2.4" IPS TFT SPI display initializes.
- Boot animation displays correctly.
- Home screen appears after boot.
- Encoder scroll changes selection.
- Encoder press opens Local Library when Local Library is selected.
- MP3 files appear in the library.
- Selecting an MP3 starts playback.
- Play/Pause button toggles playback state.
- Volume Up/Down commands change ALSA mixer volume when a mixer is exposed.
- Back exits toward the previous screen.
- Long power press requests graceful shutdown.

## Failure Modes

| Symptom | Diagnosis |
|---|---|
| Display black | Check SPI enabled, VCC, GND, GPIO24/25/26, controller compatibility |
| Display color blocks but no text | Rebuild latest firmware; run `sudo ./build/shaer_diag display` |
| Encoder silent | Run `sudo ./build/shaer_diag gpio`; verify GPIO pulls to ground |
| No MP3 files | Confirm files are under `/var/lib/shaer/music` and end in `.mp3` |
| No audio | Check `aplay -l`, `speaker-test`, `dtoverlay=hifiberry-dac`, DAC power/GND |
| Volume unchanged | PCM5102A may expose no mixer; confirm with `amixer scontrols` |
| Random reboot | Power supply sag; return to known-good USB/bench supply |

## Pass Criteria

- [ ] All diagnostics compile.
- [ ] TFT display diagnostic passes.
- [ ] TFT display initializes during full app launch.
- [ ] Boot animation displays correctly.
- [ ] Encoder/GPIO diagnostics pass.
- [ ] Audio tone is audible.
- [ ] SD-card scan finds MP3 files.
- [ ] Full app launches.
- [ ] Home screen renders.
- [ ] Library navigation works.
- [ ] MP3 playback starts through PCM5102A.
- [ ] Volume command does not crash and updates mixer where available.
- [ ] Shutdown path exits cleanly or powers off via long press.
- [ ] Full power-on to safe-shutdown flow passes for 10 consecutive boot cycles with no crashes and no manual intervention beyond intended controls.
