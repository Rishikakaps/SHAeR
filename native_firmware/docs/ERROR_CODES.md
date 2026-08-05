# SHAeR Error Codes V1

Status: Required before hardware testing.

All diagnostic popups and logs must include a stable code. Text may change; codes must not change meaning inside V1.

| Code | Meaning | User Recovery |
|---|---|---|
| `OK` | No error | None |
| `E001` | Battery gauge/battery missing or unreadable | Check 18650, holder, IP5306, MAX17048 wiring |
| `E002` | Spotify authorization/login problem | Re-authenticate in companion app |
| `E003` | DAC/audio output initialization failed | Check PCM5102A power/I2S wiring |
| `E004` | SD card/storage problem | Run SD card test/export logs |
| `E005` | Wi-Fi unavailable or disconnected | Open Local Library; fix Wi-Fi in settings |
| `E006` | Bluetooth unavailable/disconnected | Reconnect headphones |
| `E007` | Display initialization failed | Run display test; check SPI/display wiring |
| `E008` | GPIO initialization failed | Check button/encoder wiring |
| `E009` | SPI initialization failed | Check SPI enablement and display wiring |
| `E010` | I2C initialization failed | Check MAX17048/I2C wiring |
| `E011` | Watchdog disabled or unavailable | Boot continues with warning |
| `E012` | Settings migration failed | Enter Safe Mode/recovery |
| `E013` | Crash loop detected | Boot Safe Mode |
| `E014` | ALSA/audio subsystem failed | Run audio test |

