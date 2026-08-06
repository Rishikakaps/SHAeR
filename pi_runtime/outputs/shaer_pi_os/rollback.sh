#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="${SHAER_INSTALL_DIR:-/opt/shaer}"
BACKUP_DIR="${1:-${SHAER_ROLLBACK_DIR:-}}"

if [[ -z "$BACKUP_DIR" ]]; then
  echo "Usage: rollback.sh <previous-install-dir>" >&2
  echo "Example: sudo SHAER_INSTALL_DIR=/opt/shaer rollback.sh /opt/shaer.previous" >&2
  exit 2
fi

if [[ ! -d "$BACKUP_DIR/outputs" ]]; then
  echo "Rollback source must contain an outputs/ directory: $BACKUP_DIR" >&2
  exit 1
fi

sudo systemctl stop shaer-pi-os.service shaer-librespot.service shaer-ble-discovery.service 2>/dev/null || true
sudo rsync -a --delete "$BACKUP_DIR/outputs/" "$INSTALL_DIR/outputs/"
sudo systemctl daemon-reload
echo "Rolled SHAeR outputs back from $BACKUP_DIR. Start mock mode with: sudo systemctl start shaer-pi-os.service"
