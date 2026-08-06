#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONPATH="$ROOT:$ROOT/shaer_backend:$ROOT/shaer_pi_os${PYTHONPATH:+:$PYTHONPATH}"

python3 -m unittest discover -s "$ROOT/shaer_backend/tests" -p "test_*.py"
python3 -m unittest discover -s "$ROOT/shaer_companion/tests" -p "test_*.py"
python3 "$ROOT/shaer_backend/diagnostics/run_diagnostics.py"

if [[ "${SHAER_HARDWARE:-0}" == "1" ]]; then
  python3 "$ROOT/shaer_backend/diagnostics/audio_test.py"
  python3 "$ROOT/shaer_backend/diagnostics/microphone_test.py"
  python3 "$ROOT/shaer_backend/diagnostics/playback_test.py"
else
  echo "hardware diagnostics skipped; set SHAER_HARDWARE=1 on assembled hardware."
fi
