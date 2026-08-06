#!/usr/bin/env bash
set -euo pipefail

THEME="${1:-auto}"
PORT="${SHAER_PORT:-${PORT:-8775}}"
HOST="${SHAER_HOST:-${HOST:-0.0.0.0}}"
PIN_A="${SHAER_PIN_A:-${PIN_A:-17}}"
PIN_B="${SHAER_PIN_B:-${PIN_B:-27}}"
PIN_OK="${SHAER_PIN_OK:-${PIN_OK:-22}}"
PIN_BACK="${SHAER_PIN_BACK:-${PIN_BACK:-23}}"
PIN_HOME="${SHAER_PIN_HOME:-${PIN_HOME:--1}}"
ALLOW_POWER="${SHAER_ENABLE_POWER:-${ALLOW_POWER:-0}}"
ALLOW_TEST_INPUT="${SHAER_ALLOW_TEST_INPUT:-${ALLOW_TEST_INPUT:-0}}"
ENABLE_GPIO="${SHAER_ENABLE_GPIO:-${ENABLE_GPIO:-1}}"

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
  --theme "$THEME"
)

if [[ "$ENABLE_GPIO" == "1" ]]; then
  ARGS+=(
    --gpio
    --pin-a "$PIN_A"
    --pin-b "$PIN_B"
    --pin-ok "$PIN_OK"
    --pin-back "$PIN_BACK"
    --pin-home "$PIN_HOME"
  )
fi

if [[ "$ALLOW_POWER" == "1" ]]; then
  ARGS+=(--allow-power)
fi

if [[ "$ALLOW_TEST_INPUT" == "1" ]]; then
  ARGS+=(--allow-test-input)
fi

python3 "${ARGS[@]}"
