# Raspberry Pi Deployment

SHAeR deploys through Git. Do not copy source files by hand.

## First-Time Pi Setup

On a clean Raspberry Pi OS Lite install:

```bash
sudo ./scripts/pi_first_time_setup.sh
```

Then clone the private repository:

```bash
sudo mkdir -p /opt
sudo git clone <private-github-url> /opt/shaer
cd /opt/shaer
make pi
sudo make install
sudo systemctl restart shaer
```

## Wi-Fi Recovery

If the Mac has moved to a different network and the Raspberry Pi is no longer reachable over SSH, connect Wi-Fi from the Pi itself:

```bash
cd ~/SHAeR
sudo ./scripts/connect_wifi.sh "Hostel Q"
```

The script prints the new `wlan0` IP address. Use that address for the next deployment:

```bash
ssh tuku@<new-ip>
```

## Normal Update

```bash
cd /opt/shaer
sudo ./update.sh
```

## Manual Development Update

```bash
cd /opt/shaer
git pull --ff-only
make
sudo make install
sudo systemctl restart shaer
```

## Persistent Paths

| Path | Purpose |
|---|---|
| `/opt/shaer` | Git checkout and installed application files |
| `/var/lib/shaer` | Databases, settings, music, themes, cache, updates, backups |
| `/var/log/shaer` | Structured JSON logs |
| `/etc/systemd/system/shaer.service` | Foreground OS service |

## Service

`shaer.service` launches आदि Vasi OS automatically at boot, restarts after crashes, and keeps Linux hidden behind the SHAeR foreground.
