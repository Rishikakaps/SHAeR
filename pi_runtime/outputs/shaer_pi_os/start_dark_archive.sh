#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 shaer_pi_os/server.py \
  --host 0.0.0.0 \
  --port 8775 \
  --theme shaer_dark_archive \
  --gpio \
  --pin-a 17 \
  --pin-b 27 \
  --pin-ok 22 \
  --pin-back 23
