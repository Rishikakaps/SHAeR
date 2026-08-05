# SHAeR Perfboard Layout V1

Status: V1 hardware freeze.

## Critical Rule

The 18650 battery holder is not mounted on the perfboard. It mounts directly to the enclosure floor or rear shell structure.

## Perfboard Contains Only

- Raspberry Pi Zero 2 W mounting/header interface.
- DAC header.
- MAX17048 fuel gauge.
- Encoder circuitry.
- Buttons.
- Connectors.
- IP5306 wiring header.
- Microphone header.

## Suggested Perfboard Zones

```text
+--------------------------------------------------+
| USB-C / IP5306 header zone                       |
| 5V/GND in from IP5306, charger status if present |
+--------------------------+-----------------------+
| Raspberry Pi Zero 2 W    | Display SPI header    |
| 40-pin access            | GPIO8/10/11/24/25/26  |
+--------------------------+-----------------------+
| Controls zone            | I2C / MAX17048 zone   |
| encoder + buttons        | GPIO2/GPIO3           |
+--------------------------+-----------------------+
| DAC quiet zone                                   |
| GPIO18/19/21, short route to 3.5 mm jack         |
+--------------------------------------------------+
| Microphone header, away from boost noise         |
+--------------------------------------------------+
```

## Placement Rules

- DAC quiet zone must be farthest from IP5306.
- Keep analog output short and mechanically protected.
- Keep IP5306 wiring header close to enclosure USB-C opening.
- Keep encoder/buttons mechanically aligned with front shell controls.
- Keep MAX17048 close enough to battery sense route but away from noisy boost output.
- Leave the Pi microSD accessible.

## Connectors

Use labeled connectors for:

- Battery/IP5306 BAT status if exposed.
- IP5306 5 V/GND output into perfboard.
- Display SPI harness.
- DAC I2S header.
- Microphone I2S header.
- Button/encoder harness if buttons live on a daughterboard.

## Breadboard Tonight

For first test, do not mount everything to perfboard permanently. Bring up in this order:

1. Pi on bench power or IP5306 5 V output.
2. Display SPI only.
3. Encoder push and one button.
4. PCM5102A I2S DAC.
5. MAX17048 battery sense.
6. Microphone.
7. Full button set.
8. Battery holder and IP5306 on final power path.

## Keep-Out Areas

- No perfboard copper under 18650 holder spring contacts.
- No analog audio trace beside IP5306 inductor.
- No display ribbon over the battery holder opening.
- No connector placed so battery removal requires unplugging it.

