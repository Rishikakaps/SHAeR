#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="${SHAER_CONFIG_DIR:-$HOME/.config/shaer}"

sudo apt-get update
sudo apt-get install -y \
  alsa-utils \
  gstreamer1.0-tools \
  gstreamer1.0-alsa \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  python3-gpiozero

install -d -m 700 "$CONFIG_DIR" "$CONFIG_DIR/Recordings"
if [[ ! -f "$CONFIG_DIR/recording.env" ]]; then
  install -m 600 "$ROOT/shaer_pi_os/recording.env.example" "$CONFIG_DIR/recording.env"
fi

sudo install -m 644 "$ROOT/shaer_pi_os/shaer-pi-os.service" /etc/systemd/system/shaer-pi-os.service
sudo systemctl daemon-reload
sudo systemctl enable --now shaer-pi-os.service

echo "Layer 15 recording runtime installed."
echo "Next: edit $CONFIG_DIR/recording.env if the ALSA capture device is not hw:MIC,0."
echo "Then run: arecord -l"
echo "Hardware microphone and DAC acceptance remain PENDING until those checks pass."
