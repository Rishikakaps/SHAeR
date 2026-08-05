# SHAeR BOM V1

Status: V1 hardware freeze. Exact purchase SKUs still marked TBD must be chosen before final enclosure lock.

| Item | V1 Requirement | Status |
|---|---|---|
| Raspberry Pi Zero 2 W | Main computer | Frozen |
| 2.4 inch SPI display | ILI9341 or ST7789, 240 x 320 | Frozen class, exact module currently Waveshare ILI9341 |
| DAC | PCM5102A I2S line out | Frozen |
| Battery cell | Protected 18650 Li-Ion, 3000 to 3500 mAh, Samsung/LG/Panasonic class | Architecture frozen, exact cell SKU TBD |
| Battery holder | Plastic single-cell 18650 holder, screw/clip mount | Architecture frozen, exact holder SKU TBD |
| Power module | IP5306 USB-C charging + boost module | Frozen IC/class, exact carrier board TBD |
| Fuel gauge | MAX17048 I2C | Frozen |
| Microphone | INMP441 or equivalent I2S MEMS microphone breakout | Frozen class, exact SKU TBD |
| Encoder | EC11 with push | Frozen class, exact SKU TBD |
| Buttons | Momentary tactile buttons | Frozen class, exact SKU TBD |
| microSD | 32 GB minimum, A1 or better | Frozen class, exact SKU TBD |
| Connectors | Serviceable headers/harnesses | TBD |
| Fuse | Resettable polyfuse on BAT+ if not covered by selected protection/module | Recommended, final value TBD |
| Fasteners | Holder screws/clips, Pi standoffs, shell screws | TBD with enclosure |

## Battery BOM Rule

Only the frozen removable 18650 holder architecture may be added to the V1 BOM.

## Procurement Priority

For tonight’s breadboard:

1. Pi Zero 2 W.
2. ILI9341/ST7789 display.
3. PCM5102A DAC.
4. IP5306 module.
5. Plastic 18650 holder.
6. Protected 18650 cell.
7. MAX17048.
8. Encoder/buttons.
9. I2S microphone.
