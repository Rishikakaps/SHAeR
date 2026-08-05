# Layer 0 Breadboard Prototype Checkpoint

Date: 2026-06-28

## Scope

Minimal runnable Raspberry Pi prototype:

- Auto-launch service.
- SPI display boot/home/library/now-playing rendering.
- EC11/button GPIO polling path.
- SD-card music scan at `/var/lib/shaer/music`.
- MP3 decode path through `mpg123`.
- PCM5102A/I2S audio overlay validation.
- ALSA volume command path.
- Safe service restart and shutdown request path.

Excluded from this checkpoint:

- Spotify.
- Advanced themes.
- Voice recording.
- Companion sync.
- Final UI from Figma.

## Compile

Build: PASS

Mac command:

```bash
make check
```

Mac compiler:

```text
Apple clang version 21.0.0 (clang-2100.1.1.101)
```

Pi command:

```bash
make pi
```

Pi compiler:

```text
g++ (Debian 14.2.0-19) 14.2.0
```

Warnings: 0 reported

Errors: 0

## Unit And Integration Tests

Result: PASS

Command:

```bash
make check
```

Observed:

```text
navigation_tests passed
web_simulator_checks passed
Ran 8 companion tests: OK
script syntax checks passed
```

## Runtime Test

Desktop runtime launch: PASS

Command:

```bash
printf 'down\nok\nquit\n' | ./build/shaer_simulator
```

Observed:

```text
Screen: Home
Time: live system clock shown
Exit: PASS
```

## Raspberry Pi Test

Deploy to Pi: PASS

Pi path:

```text
/home/tuku/SHAeR
```

Compiled on Pi: PASS

Executed on Pi: PASS

Auto-launch service:

```text
ActiveState=active
SubState=running
NRestarts=0
```

## Diagnostics

Display:

```text
display_test: PASS
```

SD card:

```text
sdcard_test: PASS music_dir="/var/lib/shaer/music" mp3_count=1
```

Audio:

```text
PCM5102A overlay visible as snd_rpi_hifiberry_dac
audio_test: PASS if tone was audible from PCM5102A output
mpg123 decoded /var/lib/shaer/music/shaer_test.mp3
```

GPIO:

```text
gpio4=1
gpio5=1
gpio6=1
gpio7=1
gpio12=1
gpio13=1
gpio16=1
```

Battery:

```text
battery_test: percent=87 charging=no
```

Wi-Fi:

```text
wlan0 present and up
```

## Hardware Configuration Applied

`/boot/firmware/config.txt` now contains:

```text
dtparam=audio=off
dtoverlay=hifiberry-dac
```

After reboot, `aplay -l` showed:

```text
card 0: sndrpihifiberry [snd_rpi_hifiberry_dac]
device 0: HifiBerry DAC HiFi pcm5102a-hifi-0
```

## Remaining Risks

- Encoder rotation needs hands-on physical confirmation while watching diagnostic output.
- Actual headphones/speaker audibility must be confirmed by the user at the breadboard.
- The complete power-on to safe-shutdown flow must still pass for 10 consecutive boot cycles with no crashes and no manual intervention beyond intended controls.
- Volume depends on ALSA mixer availability; PCM5102A may expose no hardware mixer.
- Display controller is treated as ILI9341-style SPI; alternate display modules need Layer 1 HAL validation.

## Git

Commit: see Git tag target.

Tag: `layer-00-breadboard-prototype`
