#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUNDLE="$ROOT/shaer_pi_os_bundle.tar.gz"
TEMPORARY="$ROOT/.shaer_pi_os_bundle.tar.gz.tmp"
CHECKSUM="$BUNDLE.sha256"
CHECKSUM_TEMPORARY="$ROOT/.shaer_pi_os_bundle.tar.gz.sha256.tmp"

cd "$ROOT"
tar \
  --exclude='.DS_Store' \
  --exclude='._*' \
  --exclude='__pycache__' \
  --exclude='*.pyc' \
  --exclude='.pytest_cache' \
  --exclude='*.egg-info' \
  --exclude='outputs/theme_validation/artifacts' \
  --exclude='outputs/shaer_companion/node_modules' \
  --exclude='outputs/shaer_companion/android' \
  --exclude='outputs/shaer_companion/dist' \
  --exclude='outputs/shaer_companion/releases' \
  --exclude='*.restore-pending' \
  -czf "$TEMPORARY" \
  pyproject.toml outputs
mv "$TEMPORARY" "$BUNDLE"
shasum -a 256 "$BUNDLE" > "$CHECKSUM_TEMPORARY"
mv "$CHECKSUM_TEMPORARY" "$CHECKSUM"

echo "Built $BUNDLE"
cat "$CHECKSUM"
