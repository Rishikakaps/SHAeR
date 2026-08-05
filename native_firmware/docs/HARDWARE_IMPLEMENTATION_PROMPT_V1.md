# SHAeR Hardware Implementation Prompt V1

Status: V1 hardware freeze.

Use this prompt for future implementation work:

Build SHAeR V1 around a Raspberry Pi Zero 2 W with a 2.4 inch SPI display, PCM5102A I2S DAC, MAX17048 fuel gauge, INMP441/SPH0645-class I2S microphone, EC11 encoder, tactile buttons, IP5306 USB-C charging/boost module, and a removable single 18650 Li-Ion cell in a plastic holder.

The battery architecture is frozen:

```text
18650 cell
  -> plastic 18650 holder mounted to enclosure floor
  -> IP5306 BAT+ / BAT-
  -> IP5306 5 V boost
  -> Raspberry Pi Zero 2 W
  -> peripherals
```

Do not design for any non-18650 battery architecture. Do not solder directly to the battery. Do not mount the battery holder on the perfboard. The battery must be replaceable after opening only the rear shell.

All code must keep using the HAL-first architecture:

- C++ firmware owns UI, state, input, playback state, power, renderer, and recovery.
- Python services own Spotify metadata/OAuth, sync, and companion backend.
- No central event bus.
- Hardware-specific code lives only in HAL backends.

Tonight’s breadboard priority:

1. Boot Pi.
2. Enable SPI, I2C, and I2S.
3. Test display color bars.
4. Test encoder/buttons.
5. Test DAC sine tone.
6. Test MAX17048 reading from 18650 holder BAT+/BAT-.
7. Test microphone capture.
8. Run SHAeR Pi bring-up app.

Docs that must be obeyed:

- `HARDWARE_FREEZE_V1.md`
- `GPIO_MASTER_TABLE.md`
- `POWER_ARCHITECTURE.md`
- `WIRING_DIAGRAM_V1.md`
- `PERFBOARD_LAYOUT_V1.md`
- `ENCLOSURE_SPEC_V1.md`
- `ASSEMBLY_GUIDE_V1.md`
- `BOM_V1.md`
