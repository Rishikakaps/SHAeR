#!/usr/bin/env bash
set -euo pipefail

PURGE_DATA=0
if [[ "${1:-}" == "--purge-data" ]]; then
  PURGE_DATA=1
fi

sudo systemctl disable --now shaer-pi-os.service shaer-librespot.service shaer-ble-discovery.service 2>/dev/null || true
sudo rm -f /etc/systemd/system/shaer-pi-os.service /etc/systemd/system/shaer-librespot.service /etc/systemd/system/shaer-ble-discovery.service
sudo systemctl daemon-reload

if [[ "$PURGE_DATA" == "1" ]]; then
  echo "Purging /etc/shaer, /var/lib/shaer, /var/cache/shaer, and /var/log/shaer."
  sudo rm -rf /etc/shaer /var/lib/shaer /var/cache/shaer /var/log/shaer
else
  echo "Services removed. Runtime data preserved in /etc/shaer, /var/lib/shaer, /var/cache/shaer, and /var/log/shaer."
fi
