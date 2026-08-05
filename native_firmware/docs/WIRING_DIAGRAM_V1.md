# SHAeR Wiring Diagram V1

Status: V1 hardware freeze. This document must match `GPIO_MASTER_TABLE.md` and `HARDWARE_FREEZE_V1.md`.

## Power Wiring

```text
18650 cell
  -> plastic 18650 holder
  -> holder BAT+ wire
  -> IP5306 BAT+

18650 cell
  -> plastic 18650 holder
  -> holder BAT- wire
  -> IP5306 BAT-

IP5306 5V OUT
  -> Pi 5V pin

IP5306 GND
  -> Pi GND
  -> shared digital ground
  -> DAC ground
  -> display ground
  -> microphone ground
```

Battery holder is not mounted on perfboard. Mount it to the enclosure floor and route a short paired BAT+/BAT- cable to the IP5306.

## Display Wiring

| Display Signal | Raspberry Pi BCM GPIO | Notes |
|---|---:|---|
| VCC | 3.3 V | Use display module requirement; avoid 5 V logic unless board explicitly supports it |
| GND | GND | Shared ground |
| DIN/MOSI | GPIO10 | SPI0 MOSI |
| CLK/SCLK | GPIO11 | SPI0 SCLK |
| CS | GPIO8 | SPI0 CE0 |
| DC/RS | GPIO25 | Display data/command |
| RST | GPIO24 | Display reset |
| BL | GPIO26 | Backlight PWM/control |

## DAC Wiring

| PCM5102A Signal | Raspberry Pi BCM GPIO | Notes |
|---|---:|---|
| VIN | 3.3 V or 5 V per board | Keep supply quiet |
| GND | GND | Star/quiet routing preferred |
| BCK | GPIO18 | I2S BCLK |
| LCK/LRCLK | GPIO19 | I2S LRCLK |
| DIN | GPIO21 | I2S DOUT from Pi |
| SCK | GND or board default | PCM5102A usually does not need MCLK |
| 3.5 mm out | Headphones/powered input | Keep analog path short |

DAC should sit near the headphone jack, away from IP5306 and battery wiring.

## Microphone Wiring

| I2S Mic Signal | Raspberry Pi BCM GPIO | Notes |
|---|---:|---|
| 3V3/VDD | 3.3 V | Digital mic power |
| GND | GND | Shared ground |
| BCLK/SCK | GPIO18 | Shared I2S clock |
| WS/LRCLK | GPIO19 | Shared I2S word select |
| SD/DOUT | GPIO20 | I2S data into Pi |
| L/R | GND or 3.3 V | Select channel per mic board |

## Fuel Gauge Wiring

| MAX17048 Signal | Raspberry Pi BCM GPIO | Notes |
|---|---:|---|
| SDA | GPIO2 | I2C SDA |
| SCL | GPIO3 | I2C SCL |
| GND | GND | Shared ground |
| VIN/VBAT sense | Battery side per module | Must read the 18650 cell, not only boosted 5 V |

If using a breakout with an onboard battery connector, adapt it carefully so it senses the 18650 holder BAT+/BAT- without adding a hidden removable battery connector requirement.

## Input Wiring

| Control | GPIO | Notes |
|---|---:|---|
| Power button sense | GPIO4 | Short/double/long press interpreted by firmware |
| Encoder A | GPIO5 | Pull-up input |
| Encoder B | GPIO6 | Pull-up input |
| Encoder push | GPIO7 | Pull-up input |
| Back / Memory Mode button | GPIO12 | Pull-up input |
| Play/Pause button | GPIO13 | Pull-up input |
| Home / Options button | GPIO16 | Pull-up input |

All switches should pull GPIO to ground when pressed.

## Cable Routing

- Battery holder to IP5306: short paired BAT+/BAT- route in its own channel.
- IP5306 to Pi: short paired 5 V/GND route.
- DAC analog output: shortest route to jack, away from power.
- Display SPI: bundled and restrained, but not crossing battery spring contacts.
- Microphone: route away from boost converter and speaker/headphone jack movement.
- Leave service slack for rear shell removal without straining battery wires.
