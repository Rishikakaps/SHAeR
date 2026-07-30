#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="${SHAER_CONFIG_DIR:-$HOME/.config/shaer}"

install -d -m 700 "$CONFIG_DIR" "$CONFIG_DIR/backups" "$CONFIG_DIR/music" "$CONFIG_DIR/updates"

if command -v bluetoothctl >/dev/null 2>&1; then
  sudo install -m 644 "$ROOT/shaer_pi_os/shaer-ble-discovery.service" /etc/systemd/system/shaer-ble-discovery.service
  sudo systemctl daemon-reload
  sudo systemctl enable --now bluetooth.service shaer-ble-discovery.service
else
  echo "PENDING: bluetoothctl is unavailable; install the bluez package for Bluetooth discovery."
fi

if command -v avahi-daemon >/dev/null 2>&1 && [[ -d /etc/avahi/services ]]; then
  sudo install -m 644 "$ROOT/shaer_pi_os/shaer-companion.service" /etc/avahi/services/shaer-companion.service
  sudo systemctl reload avahi-daemon || sudo systemctl restart avahi-daemon
else
  echo "PENDING: Avahi is unavailable; install avahi-daemon for automatic Wi-Fi discovery."
fi

echo "Layer 14 companion files are installed."
echo "The Android companion can now find SHAeR over Bluetooth."
echo "Browser fallback: http://shaer.local:8775/companion"
