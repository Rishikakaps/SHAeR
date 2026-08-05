#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Run from /opt/shaer as root: sudo ./scripts/rollback_update.sh"
  exit 1
fi

latest_backup="$(find /var/lib/shaer/backups -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort | tail -n 1)"

if [ -z "$latest_backup" ]; then
  echo "No SHAeR backup found under /var/lib/shaer/backups."
  exit 1
fi

if [ -f "$latest_backup/shaer-data.tar.gz" ]; then
  tar -C /var/lib -xzf "$latest_backup/shaer-data.tar.gz"
  chown -R shaer:shaer /var/lib/shaer || true
fi

make pi
./scripts/install_adi_vasi_os.sh
systemctl restart shaer.service

echo "Rolled back persistent data from $latest_backup."
