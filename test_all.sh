#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUNDLED_PYTHON="$HOME/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3"
BUNDLED_NODE="$HOME/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node"
PYTHON="${SHAER_PYTHON:-$(command -v python3 || true)}"
NODE="${SHAER_NODE:-$(command -v node || true)}"

[[ -n "$PYTHON" ]] || { [[ -x "$BUNDLED_PYTHON" ]] && PYTHON="$BUNDLED_PYTHON"; }
[[ -n "$NODE" ]] || { [[ -x "$BUNDLED_NODE" ]] && NODE="$BUNDLED_NODE"; }
[[ -n "$PYTHON" ]] || { echo "Python 3 is required." >&2; exit 1; }
[[ -n "$NODE" ]] || { echo "Node.js is required for theme validation." >&2; exit 1; }

echo "[1/5] Native C++ firmware and simulator tests"
make -C "$ROOT/native_firmware" check

echo "[2/5] Pi runtime backend tests"
PYTHONPYCACHEPREFIX=/tmp/shaer-unified-pycache \
PYTHONPATH="$ROOT/pi_runtime/outputs:$ROOT/pi_runtime/outputs/shaer_backend" \
  "$PYTHON" -m unittest discover -s "$ROOT/pi_runtime/outputs/shaer_backend/tests"

echo "[3/5] Companion/security tests"
PYTHONPYCACHEPREFIX=/tmp/shaer-unified-pycache \
PYTHONPATH="$ROOT/pi_runtime/outputs:$ROOT/pi_runtime/outputs/shaer_backend:$ROOT/pi_runtime/outputs/shaer_pi_os" \
  "$PYTHON" -m unittest discover -s "$ROOT/pi_runtime/outputs/shaer_companion/tests"

PYTHONPYCACHEPREFIX=/tmp/shaer-unified-pycache \
PYTHONPATH="$ROOT/pi_runtime/outputs/shaer_backend" \
  "$PYTHON" "$ROOT/pi_runtime/outputs/shaer_backend/diagnostics/run_diagnostics.py"

echo "[4/5] Shared companion JavaScript contracts"
(cd "$ROOT/pi_runtime/outputs/shaer_companion" && "$NODE" --test tests-js/*.test.mjs)

echo "[5/5] Theme render/state validation"
"$NODE" "$ROOT/pi_runtime/outputs/theme_validation/theme-validation.mjs" --no-baseline

echo "All host-side SHAeR validation passed."
