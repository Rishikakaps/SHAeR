#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Run on Raspberry Pi OS Lite as root: sudo ./scripts/pi_first_time_setup.sh"
  exit 1
fi

PACKAGES=(
  git
  cmake
  make
  gcc
  g++
  pkg-config
  python3
  python3-pip
  python3-venv
  sqlite3
  libsqlite3-dev
  alsa-utils
  libasound2-dev
  flac
  mpg123
  i2c-tools
  python3-smbus
  bluez
  bluetooth
  libbluetooth-dev
  rfkill
  rsync
  iw
  wireless-tools
  wpasupplicant
  curl
  ca-certificates
)

missing=()
for package in "${PACKAGES[@]}"; do
  if ! dpkg -s "$package" >/dev/null 2>&1; then
    missing+=("$package")
  fi
done

if [ "${#missing[@]}" -gt 0 ]; then
  apt-get update
  apt-get install -y "${missing[@]}"
else
  echo "All SHAeR system packages are already installed."
fi

if command -v raspi-config >/dev/null 2>&1; then
  raspi-config nonint do_spi 0 || true
  raspi-config nonint do_i2c 0 || true
fi

if ! id shaer >/dev/null 2>&1; then
  useradd --system --home /var/lib/shaer --shell /usr/sbin/nologin shaer
fi

usermod -aG audio,bluetooth,gpio,i2c,spi shaer 2>/dev/null || true

install -d -o shaer -g shaer /var/lib/shaer /var/log/shaer

echo "Raspberry Pi OS Lite is ready for SHAeR. Clone the private repository into /opt/shaer, then run make pi and sudo make install."
