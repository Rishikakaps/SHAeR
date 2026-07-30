#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
VERSION="$(cat "$ROOT/VERSION")"
ARCHIVE="$ROOT/releases/shaer-unified-$VERSION-source.tar.gz"
TEMPORARY="$ARCHIVE.tmp"

cd "$ROOT"
export COPYFILE_DISABLE=1
tar \
  --exclude='.DS_Store' \
  --exclude='._*' \
  --exclude='build' \
  --exclude='releases' \
  --exclude='__pycache__' \
  --exclude='*.pyc' \
  --exclude='.pytest_cache' \
  --exclude='node_modules' \
  --exclude='pi_runtime/outputs/shaer_companion/android/.gradle' \
  --exclude='pi_runtime/outputs/shaer_companion/android/app/build' \
  --exclude='pi_runtime/outputs/shaer_companion/android/build' \
  --exclude='pi_runtime/outputs/shaer_companion/android/capacitor-cordova-android-plugins/build' \
  --exclude='pi_runtime/outputs/shaer_companion/releases' \
  --exclude='pi_runtime/outputs/shaer_companion/dist' \
  --exclude='pi_runtime/outputs/theme_validation/artifacts' \
  --exclude='pi_runtime/shaer_pi_os_bundle.tar.gz' \
  -czf "$TEMPORARY" \
  VERSION README.md NEXT_CHAT_START_HERE.md INTEGRATION_MAP.md PROVENANCE.md \
  build_all.sh test_all.sh package_unified.sh native_firmware pi_runtime
mv "$TEMPORARY" "$ARCHIVE"
(cd "$ROOT/releases" && shasum -a 256 "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")
echo "Built $ARCHIVE"
cat "$ARCHIVE.sha256"
