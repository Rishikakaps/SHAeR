# SHAeR Assembly Guide V1

Status: V1 hardware freeze.

## Assembly Philosophy

Build serviceability into the first prototype. Battery installation is one of the final steps. No component should require battery removal for servicing except the battery itself.

## Assembly Order

1. Inspect enclosure and clean printed supports.
2. Install threaded inserts or standoffs.
3. Mount display into front shell.
4. Mount buttons and encoder.
5. Mount DAC near headphone jack.
6. Mount Pi/perfboard assembly.
7. Mount microphone in its acoustic position.
8. Mount IP5306 near USB-C opening.
9. Mount plastic 18650 holder to enclosure floor/rear shell.
10. Route battery holder BAT+/BAT- to IP5306, but do not insert battery yet.
11. Route display, controls, DAC, mic, and I2C harnesses.
12. Verify polarity and continuity.
13. Power from current-limited bench supply or USB-C/IP5306 without battery if possible.
14. Run display, button, encoder, audio, I2C, and SD tests.
15. Insert 18650 cell as one of the final steps.
16. Close rear shell.
17. Run full recovery test menu.

## Battery Installation

1. Confirm SHAeR is shut down.
2. Confirm USB-C is disconnected unless intentionally charging.
3. Confirm holder polarity.
4. Insert protected 18650 into holder.
5. Confirm the cell is retained firmly.
6. Close rear shell.
7. Run Battery status recovery test.

## Battery Replacement

1. Long press power.
2. Wait for slow goodbye and Linux shutdown.
3. Disconnect USB-C.
4. Open rear shell only.
5. Remove 18650 from holder.
6. Insert replacement with correct polarity.
7. Close rear shell.
8. Boot and run Battery status.

## Service Rules

- Do not solder with the 18650 installed.
- Do not probe unknown power points with the battery installed during early bring-up.
- Do not let the rear shell hang from battery wires.
- Do not route wires across spring contacts.

## Final Checks

- Battery removable without moving Pi.
- DAC jack accessible and quiet.
- USB-C charging port aligned.
- Display does not flicker when gently moving battery wires.
- Encoder and buttons do not reset Pi.
- Battery percentage appears in recovery mode.

