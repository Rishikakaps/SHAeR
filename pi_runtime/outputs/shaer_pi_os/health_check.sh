#!/usr/bin/env bash
set -euo pipefail

HOST="${SHAER_HEALTH_HOST:-${SHAER_HOST:-127.0.0.1}}"
PORT="${SHAER_PORT:-8775}"
URL="http://${HOST}:${PORT}/api/v1/device/discovery"

if ! command -v curl >/dev/null 2>&1; then
  echo "curl is required for SHAeR health checks." >&2
  exit 1
fi

response="$(curl --fail --silent --show-error --max-time 5 "$URL")"
if printf '%s\n' "$response" | python3 -c 'import json, sys; data=json.load(sys.stdin); raise SystemExit(0 if data.get("ok") is True else 1)'; then
  echo "shaer health ok: $URL"
else
  echo "shaer health unexpected response from $URL" >&2
  printf '%s\n' "$response" >&2
  exit 1
fi
