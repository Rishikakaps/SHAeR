#!/usr/bin/env bash
set -euo pipefail

THEME="${1:-auto}"
PORT="${PORT:-8775}"
HOST="${HOST:-0.0.0.0}"
PIN_A="${PIN_A:-17}"
PIN_B="${PIN_B:-27}"
PIN_OK="${PIN_OK:-22}"
PIN_BACK="${PIN_BACK:-23}"
PIN_HOME="${PIN_HOME:--1}"
ALLOW_POWER="${ALLOW_POWER:-0}"
ALLOW_TEST_INPUT="${ALLOW_TEST_INPUT:-0}"

if [[ -f "${HOME}/.config/shaer/spotify.env" ]]; then
  set -a
  source "${HOME}/.config/shaer/spotify.env"
  set +a
fi

cd "$(dirname "$0")/.."
ARGS=(
  shaer_pi_os/server.py
  --host "$HOST" \
  --port "$PORT" \
  --theme "$THEME" \
  --gpio \
  --pin-a "$PIN_A" \
  --pin-b "$PIN_B" \
  --pin-ok "$PIN_OK" \
  --pin-back "$PIN_BACK" \
  --pin-home "$PIN_HOME"
)

if [[ "$ALLOW_POWER" == "1" ]]; then
  ARGS+=(--allow-power)
fi

if [[ "$ALLOW_TEST_INPUT" == "1" ]]; then
  ARGS+=(--allow-test-input)
fi

python3 "${ARGS[@]}"
