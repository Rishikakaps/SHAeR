# SHAeR Scripts

These scripts make the Raspberry Pi behave like an appliance instead of a manually managed Linux box.

## First-Time Pi Setup

```bash
sudo ./scripts/pi_first_time_setup.sh
```

Installs compiler, Git, SQLite, ALSA, I2C, SPI, Bluetooth, Wi-Fi, and Python dependencies required by SHAeR.

## Connect Wi-Fi On The Pi

When SHAeR is not reachable over SSH because it is on the wrong network, run this directly on the Raspberry Pi:

```bash
cd ~/SHAeR
sudo ./scripts/connect_wifi.sh "Hostel Q"
```

For an open network:

```bash
sudo ./scripts/connect_wifi.sh "Hostel Q" --open
```

The script uses NetworkManager when available, falls back to `wpa_supplicant`, and prints the new `wlan0` IP address.

## Install

```bash
make pi
sudo make install
sudo systemctl restart shaer
```

`install_adi_vasi_os.sh` installs आदि Vasi OS into `/opt/shaer`, creates persistent folders under `/var/lib/shaer`, installs `shaer.service`, and hides the normal Raspberry Pi user experience.

## Update

```bash
sudo ./update.sh
```

Backs up persistent data, pulls the latest private Git repository state, compiles, installs, restarts `shaer.service`, and rolls back on failure.
