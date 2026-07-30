#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
RELEASES="$ROOT/releases"

mkdir -p "$RELEASES"

echo "[1/4] Building native SHAeR simulator and tests..."
make -C "$ROOT/native_firmware" all

echo "[2/4] Packaging native firmware source..."
make -C "$ROOT/native_firmware" package
cp "$ROOT/native_firmware/releases/shaer-"*.tar.gz "$RELEASES/"

echo "[3/4] Building Raspberry Pi runtime bundle..."
bash "$ROOT/pi_runtime/build_pi_bundle.sh"
cp "$ROOT/pi_runtime/shaer_pi_os_bundle.tar.gz" "$RELEASES/shaer-pi-runtime-0.16.0.tar.gz"
(cd "$RELEASES" && shasum -a 256 "shaer-pi-runtime-0.16.0.tar.gz" > "shaer-pi-runtime-0.16.0.tar.gz.sha256")

echo "[4/4] Packaging companion PWA and Android APK..."
(cd "$ROOT/pi_runtime/outputs/shaer_companion" && npm run package:pwa && npm run android:apk)
cp "$ROOT/pi_runtime/outputs/shaer_companion/releases/shaer-companion-pwa-0.17.0.zip"* "$RELEASES/"
cp "$ROOT/pi_runtime/outputs/shaer_companion/releases/SHAeR-Companion-0.17.0-debug.apk"* "$RELEASES/"

bash "$ROOT/package_unified.sh"

echo "SHAeR combined artifacts:"
ls -lh "$RELEASES"
