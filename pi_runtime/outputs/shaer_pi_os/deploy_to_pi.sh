#!/usr/bin/env bash
set -euo pipefail

PI_HOST="${1:?Usage: deploy_to_pi.sh <pi-host-or-ip> [pi-user]}"
PI_USER="${2:-${SHAER_PI_USER:-${USER}}}"
TARGET="${PI_USER}@${PI_HOST}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUNDLE="$ROOT/shaer_pi_os_bundle.tar.gz"
REMOTE_DIR="${SHAER_REMOTE_DIR:-shaer}"
case "$REMOTE_DIR" in
  /*) REMOTE_INSTALL="$REMOTE_DIR" ;;
  *) REMOTE_INSTALL="~/$REMOTE_DIR" ;;
esac

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
  case '"$REMOTE_DIR"' in
    /*) remote_dir='"$REMOTE_DIR"' ;;
    *) remote_dir="$HOME/'"$REMOTE_DIR"'" ;;
  esac
  mkdir -p "$remote_dir"
  tar -xzf /tmp/shaer_pi_os_bundle.tar.gz -C "$remote_dir"
  chmod +x "$remote_dir"/outputs/shaer_pi_os/*.sh "$remote_dir"/outputs/shaer_pi_os/spotify_setup.py
  cd "$remote_dir"
  python3 -c "import sys; raise SystemExit(0 if sys.version_info >= (3, 10) else 1)"
  PYTHONPATH=outputs:outputs/shaer_backend:outputs/shaer_pi_os python3 -m unittest discover -s outputs/shaer_backend/tests -p "test_*.py"
  PYTHONPATH=outputs:outputs/shaer_backend:outputs/shaer_pi_os python3 -m unittest discover -s outputs/shaer_companion/tests -p "test_*.py"
  PYTHONPATH=outputs/shaer_backend python3 outputs/shaer_backend/diagnostics/run_diagnostics.py
'

echo "Files and tests are on the Pi. Install/restart services with:"
echo "  ssh -t $TARGET '${REMOTE_INSTALL}/outputs/shaer_pi_os/bootstrap_bookworm.sh --mock-hardware --install-dir ${REMOTE_INSTALL} --enable-service'"
