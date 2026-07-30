#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Run from /opt/shaer as root: sudo ./update.sh"
  exit 1
fi

if [ ! -d .git ]; then
  echo "update.sh requires /opt/shaer to be a Git checkout."
  exit 1
fi

timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
backup_root="/var/lib/shaer/backups/$timestamp"
previous_revision="$(git rev-parse HEAD)"

mkdir -p "$backup_root"

if [ -d /var/lib/shaer ]; then
  tar -C /var/lib -czf "$backup_root/shaer-data.tar.gz" shaer
fi

rollback() {
  trap - ERR
  echo "Update failed. Rolling back to $previous_revision."
  git reset --hard "$previous_revision"
  if [ -f "$backup_root/shaer-data.tar.gz" ]; then
    tar -C /var/lib -xzf "$backup_root/shaer-data.tar.gz"
    chown -R shaer:shaer /var/lib/shaer || true
  fi
  make pi
  ./scripts/install_adi_vasi_os.sh
  systemctl restart shaer.service || true
  exit 1
}

trap rollback ERR

git pull --ff-only
make pi
./scripts/install_adi_vasi_os.sh
systemctl restart shaer.service
systemctl is-active --quiet shaer.service

trap - ERR
echo "SHAeR updated successfully. Backup stored at $backup_root."
