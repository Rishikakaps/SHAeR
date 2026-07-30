#!/usr/bin/env bash
set -euo pipefail

PI_HOST="${1:?Usage: deploy_to_pi.sh <pi-host-or-ip> [pi-user]}"
PI_USER="${2:-aditya}"
TARGET="${PI_USER}@${PI_HOST}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUNDLE="$ROOT/shaer_pi_os_bundle.tar.gz"

if [[ ! -f "$BUNDLE" ]]; then
  echo "Missing $BUNDLE. Rebuild the bundle before deployment." >&2
  exit 1
fi

echo "Checking SSH access to $TARGET..."
ssh -o BatchMode=yes -o ConnectTimeout=8 "$TARGET" "printf 'SHAeR Pi reachable\\n'"

echo "Copying SHAeR bundle..."
scp "$BUNDLE" "$TARGET:/tmp/shaer_pi_os_bundle.tar.gz"

echo "Unpacking and running backend verification..."
ssh "$TARGET" '
  set -eu
  mkdir -p "$HOME/shaer"
  tar -xzf /tmp/shaer_pi_os_bundle.tar.gz -C "$HOME/shaer"
  chmod +x "$HOME/shaer/outputs/shaer_pi_os/"*.sh "$HOME/shaer/outputs/shaer_pi_os/spotify_setup.py"
  cd "$HOME/shaer"
  python3 -c "import sys; raise SystemExit(0 if sys.version_info >= (3, 10) else 1)"
  PYTHONPATH=outputs:outputs/shaer_backend:outputs/shaer_pi_os python3 -m unittest discover -s outputs/shaer_backend/tests -p "test_*.py"
  PYTHONPATH=outputs:outputs/shaer_backend:outputs/shaer_pi_os python3 -m unittest discover -s outputs/shaer_companion/tests -p "test_*.py"
  PYTHONPATH=outputs/shaer_backend python3 outputs/shaer_backend/diagnostics/run_diagnostics.py
'

echo "Files and tests are on the Pi. Install/restart services with:"
echo "  ssh -t $TARGET '~/shaer/outputs/shaer_pi_os/install_layer12.sh'"
echo "  ssh -t $TARGET '~/shaer/outputs/shaer_pi_os/install_layer15.sh'"
