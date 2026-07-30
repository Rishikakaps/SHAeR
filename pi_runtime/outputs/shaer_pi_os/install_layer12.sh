#!/usr/bin/env bash
set -euo pipefail

OUTPUTS_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG_DIR="${HOME}/.config/shaer"

if ! command -v librespot >/dev/null 2>&1; then
  echo "librespot is required. Install a Raspberry Pi build at /usr/local/bin/librespot, then rerun this installer." >&2
  exit 1
fi

mkdir -p "$CONFIG_DIR" "${HOME}/.cache/shaer/librespot" "${HOME}/.cache/shaer/spotify"
chmod 700 "$CONFIG_DIR"

if [[ ! -f "$CONFIG_DIR/spotify.env" ]]; then
  cp "$OUTPUTS_DIR/shaer_pi_os/spotify.env.example" "$CONFIG_DIR/spotify.env"
  chmod 600 "$CONFIG_DIR/spotify.env"
  echo "Created $CONFIG_DIR/spotify.env. Add your Spotify client ID before starting SHAeR."
fi

sudo cp "$OUTPUTS_DIR/shaer_pi_os/shaer-librespot.service" /etc/systemd/system/shaer-librespot.service
sudo cp "$OUTPUTS_DIR/shaer_pi_os/shaer-pi-os.service" /etc/systemd/system/shaer-pi-os.service
sudo cp "$OUTPUTS_DIR/shaer_pi_os/shaer-power-sudoers" /etc/sudoers.d/shaer-power
sudo chmod 440 /etc/sudoers.d/shaer-power
sudo visudo -cf /etc/sudoers.d/shaer-power
sudo systemctl daemon-reload
sudo systemctl enable shaer-librespot.service shaer-pi-os.service

echo "Layer 12 services installed. After editing spotify.env, run:"
echo "  sudo systemctl restart shaer-librespot shaer-pi-os"
