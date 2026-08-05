#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  sudo ./scripts/connect_wifi.sh "SSID"
  sudo ./scripts/connect_wifi.sh "SSID" --open

Prompts for the Wi-Fi password unless --open is used.
Prints wlan0 status and IP address after connecting.
USAGE
}

if [ "$(id -u)" -ne 0 ]; then
  echo "Run on SHAeR/Raspberry Pi as root: sudo ./scripts/connect_wifi.sh \"SSID\""
  exit 1
fi

if [ "${1:-}" = "" ] || [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 0
fi

SSID="$1"
OPEN_NETWORK="false"
if [ "${2:-}" = "--open" ]; then
  OPEN_NETWORK="true"
fi

PASSWORD=""
if [ "$OPEN_NETWORK" != "true" ]; then
  printf "Password for '%s': " "$SSID"
  read -r -s PASSWORD
  printf "\n"
fi

echo "[wifi] Target SSID: $SSID"

rfkill unblock wifi || true
ip link set wlan0 up || true

if command -v nmcli >/dev/null 2>&1 && systemctl is-active --quiet NetworkManager 2>/dev/null; then
  echo "[wifi] Using NetworkManager."
  nmcli radio wifi on || true
  if [ "$OPEN_NETWORK" = "true" ]; then
    nmcli dev wifi connect "$SSID" ifname wlan0
  else
    nmcli dev wifi connect "$SSID" password "$PASSWORD" ifname wlan0
  fi
else
  echo "[wifi] Using wpa_supplicant fallback."
  WPA_CONF="/etc/wpa_supplicant/wpa_supplicant.conf"
  install -d -m 755 "$(dirname "$WPA_CONF")"
  if [ -f "$WPA_CONF" ]; then
    cp "$WPA_CONF" "$WPA_CONF.shaer.bak"
  else
    cat >"$WPA_CONF" <<'EOF'
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=IN
EOF
  fi

  TMP_BLOCK="$(mktemp)"
  if [ "$OPEN_NETWORK" = "true" ]; then
    cat >"$TMP_BLOCK" <<EOF

network={
    ssid="$SSID"
    key_mgmt=NONE
}
EOF
  else
    wpa_passphrase "$SSID" "$PASSWORD" >"$TMP_BLOCK"
  fi

  awk -v ssid="$SSID" '
    BEGIN { skip=0 }
    $0 ~ /^network=\{/ { block=$0 "\n"; skip=1; next }
    skip {
      block = block $0 "\n"
      if ($0 ~ /^\}/) {
        if (block !~ "ssid=\"" ssid "\"") printf "%s", block
        skip=0
      }
      next
    }
    { print }
  ' "$WPA_CONF" >"$WPA_CONF.tmp"
  cat "$TMP_BLOCK" >>"$WPA_CONF.tmp"
  mv "$WPA_CONF.tmp" "$WPA_CONF"
  chmod 600 "$WPA_CONF"
  rm -f "$TMP_BLOCK"

  systemctl restart wpa_supplicant 2>/dev/null || true
  systemctl restart dhcpcd 2>/dev/null || true
  wpa_cli -i wlan0 reconfigure 2>/dev/null || true
fi

echo "[wifi] Waiting for DHCP..."
for _ in $(seq 1 20); do
  IP="$(ip -4 -o addr show wlan0 | awk '{print $4}' | cut -d/ -f1 | head -n1)"
  if [ -n "$IP" ]; then
    echo "[wifi] Connected. wlan0 IP: $IP"
    iw dev wlan0 link || true
    exit 0
  fi
  sleep 1
done

echo "[wifi] No IPv4 address on wlan0 yet."
echo "[wifi] Diagnostics:"
ip addr show wlan0 || true
iw dev wlan0 link || true
exit 2
