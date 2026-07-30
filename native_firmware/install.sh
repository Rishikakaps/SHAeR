#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Run from the repository root as root: sudo ./install.sh"
  exit 1
fi

if [ ! -f Makefile ] || [ ! -d firmware ]; then
  echo "install.sh must be run from the SHAeR repository root."
  exit 1
fi

./scripts/pi_first_time_setup.sh
make pi
./scripts/install_adi_vasi_os.sh

echo "SHAeR install complete. Restarting foreground service."
systemctl restart shaer.service
