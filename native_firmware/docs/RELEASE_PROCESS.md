# Release Process

## Prepare

1. Update `VERSION`.
2. Update `CHANGELOG.md`.
3. Run `make check`.
4. Run companion tests if the companion changed.
5. Run `make pi` on Raspberry Pi OS or the Pi itself.
6. Run `make package`.

## Tag

```bash
git tag -a v$(cat VERSION) -m "SHAeR $(cat VERSION)"
git push origin v$(cat VERSION)
```

## Install On Pi

```bash
cd /opt/shaer
git pull --ff-only
make
sudo make install
sudo systemctl restart shaer
```

## Update On Pi

```bash
cd /opt/shaer
sudo ./update.sh
```

## Rollback

`update.sh` stores backups under `/var/lib/shaer/backups/`. If build or service restart fails, it restores the previous Git revision and persistent databases/settings from the newest backup.

## Health Check

After a release install or update:

```bash
systemctl status shaer --no-pager
journalctl -u shaer -n 80 --no-pager
```

The device should boot to Home after cold boot. It should not resume directly into Now Playing.
