# SHAeR Breadboard Bring-Up Tonight

Status: practical V1 hardware test guide.

This uses the same C++ SHAeR app core as the simulator. The Pi-specific code lives in `firmware/hal/pi/`.

## Hardware Assumption

Use the frozen V1 battery architecture:

```text
18650 cell
  -> plastic 18650 holder
  -> IP5306 BAT+ / BAT-
  -> IP5306 5 V output
  -> Raspberry Pi Zero 2 W
```

For first boot, using a known-good USB power supply or current-limited bench supply is safer than immediately running from the 18650 path. Add the 18650 holder/IP5306 path after display, input, and audio are known-good.

## Pi OS Interfaces To Enable

Enable:

- SPI for display.
- I2C for MAX17048.
- I2S/audio overlay for PCM5102A.
- SSH if copying files over network.

Typical `/boot/firmware/config.txt` additions:

```text
dtparam=spi=on
dtparam=i2c_arm=on
dtparam=audio=off
dtoverlay=hifiberry-dac
```

Reboot after editing.

## Breadboard Order

1. Boot Pi from normal USB power.
2. Build `shaer_pi_bringup`.
3. Wire display only and confirm color blocks.
4. Wire encoder push and Back/Play/Options buttons.
5. Wire PCM5102A and confirm sine tone.
6. Wire MAX17048 to read the 18650 holder BAT+/BAT-.
7. Wire microphone.
8. Move to IP5306 + 18650 holder power path.

## Build On The Pi

From the project folder:

```bash
make pi
```

Run:

```bash
sudo ./build/shaer_pi_bringup
```

Root/sudo is currently used because the bring-up HAL touches `/sys/class/gpio`, `/dev/spidev0.0`, and `/dev/i2c-1`.

## What The Bring-Up App Does

- Initializes ILI9341-style SPI display.
- Draws theme-colored blocks for current app state.
- Reads GPIO buttons/encoder using the frozen GPIO table.
- Reads MAX17048 percentage when present.
- Reports Bluetooth connected state using `bluetoothctl`.
- Plays quick ALSA sine test tones through `speaker-test`.
- Uses the same SHAeR state machine as the desktop simulator.

## Power Button Behavior

| Action | Meaning |
|---|---|
| Short press | Battery saver toggle |
| Two quick presses | Sleep |
| Long press | Slow goodbye, then Linux shutdown |

## Display Wiring Quick Check

| Display | Pi GPIO |
|---|---:|
| DIN/MOSI | GPIO10 |
| CLK/SCLK | GPIO11 |
| CS | GPIO8 |
| DC | GPIO25 |
| RST | GPIO24 |
| BL | GPIO26 |
| VCC | 3.3 V unless module specifies otherwise |
| GND | GND |

## Audio Wiring Quick Check

| PCM5102A | Pi GPIO |
|---|---:|
| BCK | GPIO18 |
| LCK/LRCLK | GPIO19 |
| DIN | GPIO21 |
| VIN | board-appropriate power |
| GND | GND |

## Battery/Fuel Gauge Quick Check

MAX17048 must sense the 18650 holder BAT+/BAT-, not the boosted 5 V rail.

| MAX17048 | Pi GPIO |
|---|---:|
| SDA | GPIO2 |
| SCL | GPIO3 |
| GND | GND |
| VBAT/VIN sense | 18650 holder BAT+/BAT- per module |

## If Something Fails

- No display: confirm SPI enabled, GPIO24/25/26 wiring, and display controller type.
- No buttons: confirm buttons pull GPIO to ground when pressed.
- No audio: confirm `dtoverlay=hifiberry-dac`, run `aplay -l`, then test `speaker-test`.
- No battery percent: confirm I2C enabled and MAX17048 address `0x36`.
- Random resets: power path is sagging; return to known-good USB/bench supply before debugging software.

