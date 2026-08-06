# SHAeR Pi Bring-Up Run Notes

Copy the SHAeR native firmware folder to the Raspberry Pi, for example as `/opt/shaer-native` or another path you configure in `systemd/shaer-pi-bringup.service`.

Build:

```bash
cd /opt/shaer-native
make pi
```

Run manually first:

```bash
sudo ./build/shaer_pi_bringup
```

Only after manual tests work, install the service:

```bash
sudo cp systemd/shaer-pi-bringup.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable shaer-pi-bringup
sudo systemctl start shaer-pi-bringup
```

Check logs:

```bash
journalctl -u shaer-pi-bringup -f
```

For tonight, manual run is safer than enabling startup immediately.
