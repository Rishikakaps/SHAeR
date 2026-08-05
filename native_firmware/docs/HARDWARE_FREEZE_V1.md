# SHAeR Hardware Freeze V1

Status: V1 hardware freeze. This is the only document allowed to define purchased components and battery architecture. GPIO ownership is defined only in `GPIO_MASTER_TABLE.md`.

## Non-Negotiable Hardware Rules

- V1 battery architecture is a single removable 18650 Li-Ion cell in a plastic holder.
- Battery holder mounts to the enclosure floor, not the perfboard.
- No soldering directly to the battery.
- Battery must be removable after opening only the rear shell.
- Components must be replaceable without desoldering from the main assembly wherever physically possible.
- Use socketed headers, screw terminals, JST-style harnesses, ribbon connectors, or short serviceable harnesses.
- No obscure display controller is allowed. V1 display controller must be ILI9341 or ST7789.
- DAC must be physically separated from charger, boost converter, USB-C, battery contacts, and high-current traces.
- Bluetooth is headphones-only in V1 and must support automatic reconnect.

## Frozen Battery Architecture

```text
Single 18650 Li-Ion Cell
  -> Plastic single-cell 18650 battery holder
  -> BAT+ / BAT- on IP5306 USB-C charging + boost module
  -> 5 V rail
  -> Raspberry Pi Zero 2 W
  -> PCM5102A DAC
  -> 2.4 inch SPI display
  -> MAX17048 fuel gauge
  -> INMP441/SPH0645 microphone
  -> GPIO peripherals
```

Battery cell:

- Single 18650 Li-Ion.
- Protected cell preferred.
- Nominal voltage: 3.7 V.
- Fully charged voltage: 4.2 V.
- Capacity: 3000 to 3500 mAh target.
- Samsung, LG, Panasonic, or equivalent reputable cell class.

Battery holder:

- Plastic single-cell 18650 holder.
- Mounted with screws or printed clips.
- Mounted directly to enclosure floor.
- Clearance around spring contacts.
- Cable channel from holder to IP5306 BAT+/BAT-.

## Component Table

| Component | Exact Manufacturer | Model | Purchase Link | Dimensions | Weight | Power | Voltage | Connector | Future Upgrade | Reason Chosen |
|---|---|---|---|---|---|---|---|---|---|---|
| Main computer | Raspberry Pi Ltd | Raspberry Pi Zero 2 W | https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/ | 65 x 30 mm | TBD from measured unit | Budget 350 to 700 mA active | 5 V input, 3.3 V GPIO | 40-pin header footprint | CM5/Zero successor adapter board | Small, Linux capable, Wi-Fi/Bluetooth built in |
| Display | Waveshare | 2.4inch LCD Module, ILI9341, SPI | https://www.waveshare.com/wiki/2.4inch_LCD_Module | 70.5 x 43.3 mm, 240 x 320 | TBD from measured unit | Backlight budget 40 to 120 mA | 3.3 V preferred | PH2.0 8-pin or service header | ST7789/IPS drop-in display if GPIO-compatible | Known controller, SPI, Pi examples |
| DAC | Adafruit | PCM5102 I2S DAC with Line Level Output, Product 6250 | https://www.adafruit.com/product/6250 | 32.5 x 20.3 x 6.2 mm | 2.8 g | Low active current, measure in V1 prototype | 3 to 5 V power, 3.3 V logic | 0.1 in header plus 3.5 mm jack | PCM5122 or better DAC board on same I2S header | High quality line output, no MCLK required |
| Battery cell | Samsung/LG/Panasonic class | Protected 18650 Li-Ion, 3000 to 3500 mAh | Final cell SKU TBD before purchase | 18 mm diameter x 65 mm length nominal, protected cells may be slightly longer | About 45 to 50 g typical | Must safely supply Pi/display/audio peak current | 3.7 V nominal, 4.2 V full | Removable pressure contacts in holder | Higher-capacity protected 18650 within holder limits | High capacity, replaceable, widely available |
| Battery holder | Generic or Keystone-class | Plastic single-cell 18650 holder | Final holder SKU TBD before CAD lock | Holder envelope TBD by selected part, budget about 21 x 77 x 20 mm | TBD | Must support expected current without heat | 1-cell holder | Leads or solder tabs to IP5306 BAT+/BAT- | Higher-quality spring holder or keyed sled | Removable cell, no direct battery soldering |
| Fuel gauge | Adafruit | MAX17048 Fuel Gauge, Product 5580 | https://www.adafruit.com/product/5580 | 25.7 x 20.3 x 7.2 mm | 2.6 g | Low I2C monitor current | Battery side 3.7 to 4.2 V, logic 3.3 V | STEMMA QT/Qwiic or header | Alternate I2C fuel gauge behind Battery HAL | Accurate battery percentage without hand-tuned voltage guessing |
| Microphone | Adafruit or equivalent | I2S MEMS microphone breakout, INMP441 preferred for V1 wiring | Final mic SKU TBD if switching from SPH0645 | Typical breakout under 20 x 15 mm | TBD | Low active current, measure in prototype | 3.3 V logic | 0.1 in header | Alternate I2S/PDM mic through Microphone HAL | Digital voice capture for Memory Mode |
| Power/charger/boost | Injoinic IC on serviceable module | IP5306 USB-C charging + boost module | Exact carrier board TBD before enclosure freeze | TBD by selected board | TBD | Must supply stable 5 V at Pi peaks | 18650 holder into BAT+/BAT-, 5 V boost out | BAT+/BAT-, USB-C charge, 5 V/GND output | Custom power PCB or higher-efficiency PMIC | One-board charging and boost path for compact prototype |
| Encoder | ALPS Alpine or equivalent | EC11 incremental rotary encoder with push | Final purchase link TBD | Typical 12 mm body | TBD | GPIO pull-up only | 3.3 V logic | JST/service harness | Optical encoder if wear becomes issue | Low-latency navigation with tactile feel |
| Buttons | Omron/ALPS/Kailh or equivalent | Momentary tactile switches | Final purchase link TBD | TBD per industrial design | TBD | GPIO pull-up only | 3.3 V logic | JST/service harness | Custom key mat or sealed tact switches | Repairable physical controls |
| Storage | SanDisk/Samsung/Kingston | microSD A1 or better, 32 GB minimum | Final purchase link TBD | microSD | negligible | Pi slot | 3.3 V internal | Pi microSD slot | 64/128 GB media | Local music, metadata, recordings, logs |
| Headphone output | Board-mounted 3.5 mm jack on DAC | 3.5 mm line output | DAC purchase link | Included on DAC | Included | Line level only | Analog | 3.5 mm jack | Add headphone amplifier only if tests require it | Keeps V1 simple and efficient |

## Components Not Yet Frozen

These cannot be placed on a final PCB or final enclosure until exact part numbers are selected:

- IP5306 carrier board.
- Protected 18650 cell SKU.
- Plastic single-cell 18650 holder SKU.
- EC11 encoder variant.
- Tactile button model.
- microSD SKU.
- Final enclosure fasteners and internal connector set.

## Internal Packaging Requirements

- Battery holder sits on the enclosure floor/rear shell side.
- Pi and perfboard sit above or beside the battery without blocking removal.
- DAC remains near headphone jack for the shortest analog path.
- IP5306 stays near USB-C opening and battery cable channel.
- Battery mass must be centered enough that SHAeR does not feel top-heavy.
- No battery spring contact may rub against the Pi, display, DAC, or microphone wiring.
- Rear shell removal must expose the battery holder directly.

## Power Budget Targets

| Mode | Target Current | Notes |
|---|---:|---|
| Boot | 500 to 800 mA peak | Wi-Fi and display active |
| Local playback, screen on | 350 to 600 mA | Depends on backlight |
| Bluetooth playback, screen on | 450 to 700 mA | Bluetooth reconnect costs extra |
| AOD | under 250 mA | Display dim, low refresh |
| Sleep | under 100 mA if possible | Pi Zero cannot deep sleep like MCU; auto shutdown matters |
| Shutdown | near zero after power latch off | Depends on power board |

## Source Notes

Raspberry Pi lists Zero 2 W as 65 x 30 mm with 2.4 GHz Wi-Fi, Bluetooth 4.2/BLE, and a 40-pin header footprint. Waveshare lists the 2.4 inch LCD as SPI, 240 x 320, ILI9341, and 70.5 x 43.3 mm. Adafruit lists PCM5102 dimensions/weight and supported 8 kHz to 384 kHz sample rates. The 18650 cell form factor is treated as the V1 battery architecture and must be measured with the selected protected cell plus holder before CAD lock.

