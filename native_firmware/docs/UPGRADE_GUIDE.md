# SHAeR Upgrade Guide V1

Status: V1 freeze draft.

## Repair Philosophy

SHAeR should be maintainable like a small instrument. Replace common failure points without redesigning the device:

- Battery replaceable without soldering.
- DAC replaceable by unplugging header/harness.
- Display replaceable by opening the front assembly and unplugging the display harness.
- Buttons replaceable as a small daughterboard or harness.
- Encoder replaceable without touching the Pi.
- Microphone replaceable from a small daughterboard.

## Software Rule

Hardware upgrades must land behind HAL interfaces. UI, navigation, Memory Mode, queue logic, and theme behavior must not import part-specific drivers.

## Replacing DAC

Requirements:

- Keep I2S BCLK, LRCLK, DOUT pins from `GPIO_MASTER_TABLE.md`.
- Keep line output isolated from charger/boost noise.
- Update `AudioOutput` implementation only if driver behavior changes.
- Run recovery Audio test after replacement.

Allowed future DACs:

- PCM5102A equivalent.
- PCM5122 board.
- Higher-quality DAC with same I2S contract.

## Replacing Battery

Requirements:

- Single 18650 Li-Ion cell.
- Protected cell preferred.
- 3000 to 3500 mAh target.
- Reputable Samsung, LG, Panasonic, or equivalent cell class.
- Fits the frozen plastic 18650 holder without compression.
- No soldering directly to the cell.
- Charge current remains safe.

After replacement:

- Open only the rear shell.
- Remove the cell from the holder.
- Insert replacement with correct polarity.
- Run Battery status recovery test.
- Let MAX17048 settle/calibrate.
- Check shutdown thresholds.

## Replacing Display

Requirements:

- ILI9341 or ST7789 for V1.
- 240 x 320 portrait target unless V2 layout work is approved.
- SPI-compatible or a new Display HAL implementation.
- Backlight controllable by firmware.

After replacement:

- Run Display test.
- Verify AOD brightness.
- Verify boot reaches Home in 6 to 8 seconds.

## Replacing Microphone

Requirements:

- Digital MEMS preferred.
- HAL must expose the same recording controls.
- Voice recordings must save as WAV and MP3.

After replacement:

- Run voice recording test.
- Check recording level, clipping, and noise.
- Confirm Memory Mode link creation.

## Replacing Buttons

Requirements:

- Momentary switches.
- 3.3 V GPIO safe.
- Debounced in input HAL.
- Physically serviceable.

After replacement:

- Run Button test.
- Verify short, double, and long power press behavior.

## Replacing Encoder

Requirements:

- Incremental rotary encoder with push.
- 3.3 V GPIO safe.
- Debounced in input HAL.

After replacement:

- Run Encoder test.
- Verify menu wrap/clamp behavior.
- Verify no accidental double actions.

## Upgrading Storage

Requirements:

- A1 or better microSD.
- 32 GB minimum.
- Keep database, logs, recordings, and cover cache migration tested.

After replacement:

- Run SD card recovery test.
- Export/import metadata backup.

## Upgrade Checklist

Before an upgrade:

- Confirm GPIO changes in `GPIO_MASTER_TABLE.md`.
- Confirm hardware changes in `HARDWARE_FREEZE_V1.md`.
- Confirm HAL implementation exists.
- Confirm recovery test exists.
- Confirm logs capture the new failure mode.

After an upgrade:

- Boot test.
- Display test.
- Button test.
- Encoder test.
- Audio test.
- Battery test.
- Bluetooth reconnect test.
- Wi-Fi test.
- SD card test.
- Export logs.
