#!/usr/bin/env bash
set -euo pipefail

FOLLOW="${1:---no-pager}"
journalctl -u shaer-pi-os.service -u shaer-librespot.service -u shaer-ble-discovery.service "$FOLLOW"
