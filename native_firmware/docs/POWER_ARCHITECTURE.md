# SHAeR Power Architecture V1

Status: V1 hardware freeze. All future hardware decisions must assume this exact battery architecture.

## Frozen Battery Architecture

V1 uses a single removable 18650 Li-Ion cell in a plastic single-cell holder.

Battery requirements:

- Single 18650 cylindrical Li-Ion cell.
- Protected cell preferred.
- Nominal voltage: 3.7 V.
- Fully charged voltage: 4.2 V.
- Capacity target: 3000 to 3500 mAh.
- Samsung, LG, Panasonic, or equivalent reputable cell class.
- No soldering directly to the cell.

Battery holder requirements:

- Plastic single-cell 18650 holder.
- Holder mounted directly to enclosure floor using screws or printed clips.
- Battery removable after opening only the rear shell.
- Clearance around spring contacts.
- Battery removal must not require removing Pi, DAC, display, perfboard, or buttons.

## Power Path

```text
18650 Li-Ion Cell
  -> Plastic 18650 Battery Holder
  -> BAT+ / BAT- on IP5306 USB-C Charging + Boost Module
  -> 5 V Rail
  -> Raspberry Pi Zero 2 W
  -> 3.3 V Rail
      -> PCM5102A DAC
      -> 2.4 inch SPI Display logic
      -> MAX17048 Fuel Gauge
      -> INMP441/SPH0645 I2S Microphone
      -> GPIO peripherals
```

The battery holder connects only to BAT+ and BAT- of the IP5306 module. The battery holder is not mounted on the perfboard.

## Charging Path

```text
USB-C Input
  -> IP5306 charger
  -> 18650 holder BAT+ / BAT-
  -> 18650 cell
```

The USB-C port should be reachable from the enclosure edge. Charging status should be exposed to firmware if the selected IP5306 carrier provides a status pin.

## Shutdown Path

Long press power button:

1. Firmware enters `SHUTDOWN`.
2. Playback fades/stops.
3. Any voice recording finalizes to WAV/MP3.
4. SQLite, settings, and logs flush.
5. Display shows the slow goodbye message.
6. Linux shutdown is requested.
7. IP5306/power latch may remove power only after Linux has committed shutdown.

Never remove the 18650 while SHAeR is writing unless it is an emergency.

## Current Path

High-current path must be short and physically separated from analog audio:

```text
18650 holder
  -> short twisted or paired BAT+/BAT- wires
  -> IP5306 BAT pads
  -> IP5306 5 V/GND output
  -> short 5 V/GND pair to Pi 5 V/GND
```

Route DAC analog output away from:

- 18650 holder contacts.
- IP5306 inductor/boost section.
- USB-C charging input.
- Display backlight wiring.
- 5 V rail.

## Perfboard Power Role

The perfboard may carry connectors and low-current distribution, but the battery holder does not mount to the perfboard.

Perfboard contains only:

- Raspberry Pi Zero 2 W mount/header.
- DAC header.
- MAX17048.
- Encoder circuitry.
- Buttons.
- Connectors.
- IP5306 wiring header.
- Microphone header.

## Fuse Recommendation

Use a protected 18650 cell if possible. Add an inline resettable polyfuse or fuse on BAT+ between holder and IP5306 if the selected holder/module does not already provide adequate protection.

Initial recommendation: 2 A hold-class resettable polyfuse, final value confirmed by measured Pi/display/audio peak current and IP5306 module rating.

## Battery Removal Procedure

1. Long press power and wait for the slow goodbye message to complete.
2. Confirm the display is off and the Pi activity LED has stopped.
3. Disconnect USB-C charging.
4. Open rear shell only.
5. Lift the 18650 from the holder using the marked positive/negative orientation.
6. Inspect spring contacts for deformation or corrosion.

## Battery Replacement Procedure

1. Use a reputable protected 18650 cell, 3000 to 3500 mAh target.
2. Confirm polarity markings on holder.
3. Insert negative end against spring contact unless the holder marking states otherwise.
4. Close rear shell.
5. Connect USB-C or short press power.
6. Run Battery status recovery test.

## Runtime Estimates

With a 3000 to 3500 mAh 18650 cell, V1 target estimates:

| Mode | Estimated Current | Runtime Target |
|---|---:|---:|
| Local playback, dim screen | 350 mA | 6 to 8 hours |
| Local playback, bright screen | 550 mA | 4 to 5.5 hours |
| Bluetooth headphones, screen on | 550 to 700 mA | 3.5 to 5 hours |
| AOD | 150 to 250 mA | 10 to 16 hours |
| Sleep software mode | 50 to 100 mA target | auto shutdown at 45 minutes if idle |

These are estimates until measured on the breadboard prototype with the selected IP5306 carrier.

## Shutdown Thresholds

| Battery | Behavior |
|---:|---|
| 20 percent | Suggest battery saver |
| 15 percent | Enter critical visual budget |
| 10 percent | Blocking low battery popup |
| 5 percent | Stop recording safely, prepare shutdown |
| 3 percent or fuel-gauge critical alert | Slow shutdown immediately |

## Battery Percentage Mapping

MAX17048 percentage is primary. Voltage fallback is only for recovery:

| Voltage | Fallback Estimate |
|---:|---:|
| 4.20 V | 100 percent |
| 4.00 V | 80 percent |
| 3.85 V | 60 percent |
| 3.75 V | 40 percent |
| 3.65 V | 20 percent |
| 3.50 V | 10 percent |
| 3.30 V | emergency shutdown |

Fuel-gauge readings must be smoothed to avoid jumping UI percentages.

## Cable Routing

- Battery wires route along an enclosure channel from holder to IP5306.
- Keep battery and 5 V wiring short, paired, and mechanically restrained.
- Keep DAC output wire shortest and quietest.
- Cross analog audio and power wires at 90 degrees if crossing is unavoidable.
- Do not place battery spring contacts under the display ribbon, DAC jack, or Pi USB port.

## Battery Safety

- Do not use damaged, dented, torn-wrap, unknown, or counterfeit cells.
- Do not reverse polarity.
- Do not charge unattended during first bring-up.
- Do not bypass cell protection casually.
- Do not allow loose metal parts near the holder.
- Add polarity markings inside the enclosure.

## Power Logs

Power events go to `logs/power.log`:

- Boot voltage and percentage.
- Charger attach/detach.
- Battery saver entry/exit.
- AOD entry/exit.
- Sleep entry/exit.
- Shutdown reason.
- Critical battery events.
- Battery removed/reinserted if detected during service mode.

