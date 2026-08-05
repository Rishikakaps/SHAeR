# SHAeR GPIO Master Table V1

Status: V1 freeze draft. This is the only document allowed to assign GPIOs. No other document may redefine a GPIO.

Pin names use Raspberry Pi BCM GPIO numbering.

| GPIO | Physical Pin | Function | Device | Direction | Notes |
|---:|---:|---|---|---|---|
| GPIO2 | 3 | I2C SDA | MAX17048 fuel gauge | bidirectional | Pull-ups on fuel gauge/STEMMA chain |
| GPIO3 | 5 | I2C SCL | MAX17048 fuel gauge | output | Keep I2C available for future sensors |
| GPIO4 | 7 | Power button wake/sense | Power button / power board | input | Short, double, long press interpreted by firmware |
| GPIO5 | 29 | Encoder A | EC11 encoder | input | Internal pull-up, debounce in HAL |
| GPIO6 | 31 | Encoder B | EC11 encoder | input | Internal pull-up, debounce in HAL |
| GPIO7 | 26 | Encoder push | EC11 encoder | input | Internal pull-up |
| GPIO8 | 24 | SPI0 CE0 | Display CS | output | Reserved for display only |
| GPIO9 | 21 | SPI0 MISO | Unused/reserved | input | Leave free unless display readback/debug needs it |
| GPIO10 | 19 | SPI0 MOSI | Display DIN | output | Display only |
| GPIO11 | 23 | SPI0 SCLK | Display CLK | output | Display only |
| GPIO12 | 32 | Button 1 | Back / Memory Mode | input | Internal pull-up |
| GPIO13 | 33 | Button 2 | Play/Pause | input | Internal pull-up |
| GPIO16 | 36 | Button 3 | Home / Options | input | Internal pull-up |
| GPIO18 | 12 | PCM/I2S BCLK | DAC and microphone shared clock | output | Also usable for PWM; reserved for I2S |
| GPIO19 | 35 | PCM/I2S LRCLK | DAC and microphone shared word select | output | Reserved for I2S |
| GPIO20 | 38 | PCM/I2S DIN | MEMS microphone data | input | Voice recording input |
| GPIO21 | 40 | PCM/I2S DOUT | PCM5102 DAC data | output | Playback output |
| GPIO22 | 15 | Power hold / charger status | IP5306 module | input/output | Exact direction depends on final carrier board |
| GPIO23 | 16 | Wi-Fi/Bluetooth recovery button or spare | Recovery/service | input | Leave accessible on test pads |
| GPIO24 | 18 | Display reset | SPI display | output | Match selected display harness |
| GPIO25 | 22 | Display data/command | SPI display | output | Display DC/RS |
| GPIO26 | 37 | Display backlight PWM | SPI display | output | Firmware controls dimming and AOD |
| GPIO27 | 13 | Recorder LED or display reset spare | Status/service | output | If display needs GPIO27 reset, move recorder LED/status LED to GPIO23 |
| GPIO14 | 8 | Reserved UART TX / future dock debug | Serial/debug | output | Leave uncommitted for bring-up unless serial console is required |
| GPIO15 | 10 | Reserved UART RX / future dock debug | Serial/debug | input | Leave uncommitted for bring-up unless serial console is required |
| GPIO17 | 11 | Reserved future haptic motor enable | Haptic/service | output | Test pad only in V1; no firmware dependency |

## Power Pins

| Pin | Function | Notes |
|---|---|---|
| 5 V | Pi power from boost | Must be stable under Wi-Fi/audio/display peaks |
| 3.3 V | Display logic, DAC logic, microphone, I2C | Check total draw before final PCB |
| GND | Shared ground | Star/quiet routing near DAC analog output |

## Reserved Interfaces

- SPI0 is display-only in V1.
- I2S/PCM is audio-only in V1.
- I2C is battery/fuel-gauge-first, with future expansion allowed.
- UART pins GPIO14/GPIO15 are left free for serial debug.
- Reserve future pads/signals for haptic motor, RGB status LED, dock detection, USB OTG detect, Hall sensor, and ambient light sensor. These are not active V1 requirements.

## Change Rule

If a GPIO must move, update this file first, then update hardware schematics, HAL pin maps, recovery tests, and docs. No code should carry a hidden pin assignment.
