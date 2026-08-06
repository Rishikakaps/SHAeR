#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: bootstrap_bookworm.sh [options]

Options:
  --dry-run                 Print actions without changing the system.
  --assume-bookworm         Allow dry-run/static checks outside Raspberry Pi OS Bookworm.
  --mock-hardware           Keep GPIO, display, audio, BLE, and power services disabled.
  --enable-gpio             Render the UI service with GPIO input enabled.
  --enable-display          Install display/browser packages. Does not start a kiosk service yet.
  --enable-audio            Install audio packages and render the librespot service.
  --enable-ble              Render the Bluetooth discovery service.
  --enable-power            Allow the server's protected power action.
  --enable-service          Enable and start rendered services. Real hardware requires explicit flags.
  --install-dir DIR         Install root. Default: /opt/shaer.
  --user USER               Runtime user. Default: shaer.
  --systemd-dir DIR         Service output dir. Default: /etc/systemd/system.
  --config-dir DIR          Config dir. Default: /etc/shaer.
  --data-dir DIR            Runtime data dir. Default: /var/lib/shaer.
  --cache-dir DIR           Runtime cache dir. Default: /var/cache/shaer.
  --skip-apt                Skip apt-get package installation.
  --help                    Show this help.
USAGE
}

DRY_RUN=0
ASSUME_BOOKWORM=0
MOCK_HARDWARE=1
ENABLE_GPIO=0
ENABLE_DISPLAY=0
ENABLE_AUDIO=0
ENABLE_BLE=0
ENABLE_POWER=0
ENABLE_SERVICE=0
SKIP_APT=0
RUNTIME_USER="${SHAER_USER:-shaer}"
RUNTIME_GROUP="${SHAER_GROUP:-$RUNTIME_USER}"
INSTALL_DIR="${SHAER_INSTALL_DIR:-/opt/shaer}"
CONFIG_DIR="${SHAER_CONFIG_DIR:-/etc/shaer}"
DATA_DIR="${SHAER_DATA_DIR:-/var/lib/shaer}"
CACHE_DIR="${SHAER_CACHE_DIR:-/var/cache/shaer}"
LOG_DIR="${SHAER_LOG_DIR:-/var/log/shaer}"
SYSTEMD_DIR="${SHAER_SYSTEMD_DIR:-/etc/systemd/system}"
HOST="${SHAER_HOST:-0.0.0.0}"
PORT="${SHAER_PORT:-8775}"
THEME="${SHAER_THEME:-auto}"
PIN_A="${SHAER_PIN_A:-17}"
PIN_B="${SHAER_PIN_B:-27}"
PIN_OK="${SHAER_PIN_OK:-22}"
PIN_BACK="${SHAER_PIN_BACK:-23}"
PIN_HOME="${SHAER_PIN_HOME:--1}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=1 ;;
    --assume-bookworm) ASSUME_BOOKWORM=1 ;;
    --mock-hardware) MOCK_HARDWARE=1; ENABLE_GPIO=0; ENABLE_DISPLAY=0; ENABLE_AUDIO=0; ENABLE_BLE=0; ENABLE_POWER=0 ;;
    --enable-gpio) MOCK_HARDWARE=0; ENABLE_GPIO=1 ;;
    --enable-display) MOCK_HARDWARE=0; ENABLE_DISPLAY=1 ;;
    --enable-audio) MOCK_HARDWARE=0; ENABLE_AUDIO=1 ;;
    --enable-ble) MOCK_HARDWARE=0; ENABLE_BLE=1 ;;
    --enable-power) MOCK_HARDWARE=0; ENABLE_POWER=1 ;;
    --enable-service) ENABLE_SERVICE=1 ;;
    --install-dir) INSTALL_DIR="${2:?Missing value for --install-dir}"; shift ;;
    --user) RUNTIME_USER="${2:?Missing value for --user}"; RUNTIME_GROUP="${SHAER_GROUP:-$RUNTIME_USER}"; shift ;;
    --systemd-dir) SYSTEMD_DIR="${2:?Missing value for --systemd-dir}"; shift ;;
    --config-dir) CONFIG_DIR="${2:?Missing value for --config-dir}"; shift ;;
    --data-dir) DATA_DIR="${2:?Missing value for --data-dir}"; shift ;;
    --cache-dir) CACHE_DIR="${2:?Missing value for --cache-dir}"; shift ;;
    --skip-apt) SKIP_APT=1 ;;
    --help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

OUTPUTS_DIR="${SHAER_OUTPUTS_DIR:-$INSTALL_DIR/outputs}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_OUTPUTS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

run() {
  if [[ "$DRY_RUN" == "1" ]]; then
    printf '+ %q' "$@"
    printf '\n'
  else
    "$@"
  fi
}

as_root() {
  if [[ "$EUID" -eq 0 ]]; then
    run "$@"
  else
    run sudo "$@"
  fi
}

validate_platform() {
  if [[ "$ASSUME_BOOKWORM" == "1" ]]; then
    if [[ "$DRY_RUN" != "1" ]]; then
      echo "--assume-bookworm is only allowed with --dry-run." >&2
      exit 1
    fi
    echo "Static platform check: assuming Raspberry Pi OS Bookworm arm64."
    return
  fi
  if [[ ! -r /etc/os-release ]]; then
    echo "Cannot read /etc/os-release; use --assume-bookworm only for dry-run checks." >&2
    exit 1
  fi
  . /etc/os-release
  if [[ "${VERSION_CODENAME:-}" != "bookworm" && "${VERSION_ID:-}" != "12" ]]; then
    echo "Expected Raspberry Pi OS/Debian Bookworm 12, found ${PRETTY_NAME:-unknown}." >&2
    exit 1
  fi
  arch="$(dpkg --print-architecture 2>/dev/null || uname -m)"
  if [[ "$arch" != "arm64" && "$arch" != "aarch64" ]]; then
    echo "Expected 64-bit arm64/aarch64, found $arch." >&2
    exit 1
  fi
}

install_packages() {
  if [[ "$SKIP_APT" == "1" ]]; then
    echo "Skipping apt packages by request."
    return
  fi
  local packages=(
    ca-certificates
    curl
    rsync
    python3
    python3-pip
    python3-venv
    python3-pil
    python3-qrcode
    avahi-daemon
    bluez
  )
  if [[ "$ENABLE_GPIO" == "1" ]]; then
    packages+=(python3-gpiozero python3-lgpio)
  fi
  if [[ "$ENABLE_AUDIO" == "1" ]]; then
    packages+=(alsa-utils gstreamer1.0-tools gstreamer1.0-alsa gstreamer1.0-plugins-base gstreamer1.0-plugins-good)
  fi
  if [[ "$ENABLE_DISPLAY" == "1" ]]; then
    packages+=("${SHAER_BROWSER_PACKAGE:-chromium-browser}")
  fi
  as_root apt-get update
  as_root apt-get install -y "${packages[@]}"
}

ensure_user_and_dirs() {
  if ! id "$RUNTIME_USER" >/dev/null 2>&1; then
    as_root useradd --system --create-home --shell /usr/sbin/nologin "$RUNTIME_USER"
  fi
  as_root install -d -m 755 "$INSTALL_DIR" "$OUTPUTS_DIR" "$LOG_DIR"
  as_root install -d -m 700 -o "$RUNTIME_USER" -g "$RUNTIME_GROUP" "$CONFIG_DIR" "$DATA_DIR" "$CACHE_DIR" "$DATA_DIR/Recordings" "$CACHE_DIR/librespot" "$CACHE_DIR/spotify" "$DATA_DIR/music" "$DATA_DIR/backups" "$DATA_DIR/updates"
  if [[ "$SOURCE_OUTPUTS_DIR" != "$OUTPUTS_DIR" ]]; then
    as_root rsync -a --delete --exclude '.DS_Store' "$SOURCE_OUTPUTS_DIR/" "$OUTPUTS_DIR/"
  fi
  as_root chown -R "$RUNTIME_USER:$RUNTIME_GROUP" "$OUTPUTS_DIR"
  if [[ ! -f "$CONFIG_DIR/shaer.env" ]]; then
    render_env | if [[ "$DRY_RUN" == "1" ]]; then cat; else sudo tee "$CONFIG_DIR/shaer.env" >/dev/null; fi
    as_root chmod 600 "$CONFIG_DIR/shaer.env"
    as_root chown "$RUNTIME_USER:$RUNTIME_GROUP" "$CONFIG_DIR/shaer.env"
  fi
}

render_env() {
  cat <<ENV
SHAER_USER=$RUNTIME_USER
SHAER_GROUP=$RUNTIME_GROUP
SHAER_INSTALL_DIR=$INSTALL_DIR
SHAER_OUTPUTS_DIR=$OUTPUTS_DIR
SHAER_CONFIG_DIR=$CONFIG_DIR
SHAER_DATA_DIR=$DATA_DIR
SHAER_CACHE_DIR=$CACHE_DIR
SHAER_LOG_DIR=$LOG_DIR
SHAER_HOST=$HOST
SHAER_PORT=$PORT
SHAER_THEME=$THEME
SHAER_MOCK_HARDWARE=$MOCK_HARDWARE
SHAER_ENABLE_GPIO=$ENABLE_GPIO
SHAER_ENABLE_DISPLAY=$ENABLE_DISPLAY
SHAER_ENABLE_AUDIO=$ENABLE_AUDIO
SHAER_ENABLE_POWER=$ENABLE_POWER
SHAER_ENABLE_BLE=$ENABLE_BLE
SHAER_ALLOW_TEST_INPUT=1
SHAER_PIN_A=$PIN_A
SHAER_PIN_B=$PIN_B
SHAER_PIN_OK=$PIN_OK
SHAER_PIN_BACK=$PIN_BACK
SHAER_PIN_HOME=$PIN_HOME
SHAER_ALSA_DEVICE=${SHAER_ALSA_DEVICE:-hw:0,0}
SHAER_MIC_DEVICE=${SHAER_MIC_DEVICE:-hw:MIC,0}
SHAER_RECORDINGS_DIR=$DATA_DIR/Recordings
SHAER_RECORDINGS_DB=$DATA_DIR/recordings.db
SHAER_RECORDING_MAX_SECONDS=${SHAER_RECORDING_MAX_SECONDS:-3600}
SHAER_RECORDING_MIN_FREE_BYTES=${SHAER_RECORDING_MIN_FREE_BYTES:-134217728}
SPOTIFY_CLIENT_ID=${SPOTIFY_CLIENT_ID:-}
SPOTIFY_REDIRECT_URI=${SPOTIFY_REDIRECT_URI:-http://127.0.0.1:8775/api/spotify/callback}
ENV
}

server_args() {
  local args=(--host "$HOST" --port "$PORT" --theme "$THEME" --allow-test-input)
  if [[ "$ENABLE_GPIO" == "1" ]]; then
    args+=(--gpio --pin-a "$PIN_A" --pin-b "$PIN_B" --pin-ok "$PIN_OK" --pin-back "$PIN_BACK" --pin-home "$PIN_HOME")
  fi
  if [[ "$ENABLE_POWER" == "1" ]]; then
    args+=(--allow-power)
  fi
  printf '%q ' "${args[@]}"
}

render_pi_service() {
  cat <<UNIT
[Unit]
Description=SHAeR Pi OS UI
After=network-online.target
Wants=network-online.target

[Service]
WorkingDirectory=$OUTPUTS_DIR
EnvironmentFile=-$CONFIG_DIR/shaer.env
EnvironmentFile=-$CONFIG_DIR/spotify.env
EnvironmentFile=-$CONFIG_DIR/recording.env
ExecStart=/usr/bin/python3 $OUTPUTS_DIR/shaer_pi_os/server.py $(server_args)
TimeoutStopSec=15
Restart=always
RestartSec=2
User=$RUNTIME_USER

[Install]
WantedBy=multi-user.target
UNIT
}

render_librespot_service() {
  cat <<UNIT
[Unit]
Description=SHAeR Spotify Connect receiver
After=network-online.target sound.target
Wants=network-online.target

[Service]
Type=simple
User=$RUNTIME_USER
EnvironmentFile=-$CONFIG_DIR/shaer.env
EnvironmentFile=-$CONFIG_DIR/spotify.env
ExecStart=/usr/local/bin/librespot --name SHAeR --backend alsa --device \${SHAER_ALSA_DEVICE} --cache $CACHE_DIR/librespot --bitrate 320
Restart=always
RestartSec=3
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
UNIT
}

render_ble_service() {
  cat <<UNIT
[Unit]
Description=SHAeR Bluetooth companion discovery
After=bluetooth.service network-online.target shaer-pi-os.service
Wants=bluetooth.service network-online.target

[Service]
Type=simple
User=$RUNTIME_USER
EnvironmentFile=-$CONFIG_DIR/shaer.env
ExecStart=/usr/bin/python3 $OUTPUTS_DIR/shaer_pi_os/shaer_ble_discovery.py --port $PORT
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
UNIT
}

write_unit() {
  local name="$1"
  local content="$2"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "Would write $SYSTEMD_DIR/$name"
    printf '%s\n' "$content"
  else
    printf '%s\n' "$content" | sudo tee "$SYSTEMD_DIR/$name" >/dev/null
  fi
}

install_services() {
  as_root install -d -m 755 "$SYSTEMD_DIR"
  write_unit shaer-pi-os.service "$(render_pi_service)"
  if [[ "$ENABLE_AUDIO" == "1" ]]; then
    write_unit shaer-librespot.service "$(render_librespot_service)"
  fi
  if [[ "$ENABLE_BLE" == "1" ]]; then
    write_unit shaer-ble-discovery.service "$(render_ble_service)"
  fi
  as_root systemctl daemon-reload
  if [[ "$ENABLE_SERVICE" == "1" ]]; then
    as_root systemctl enable --now shaer-pi-os.service
    if [[ "$ENABLE_AUDIO" == "1" ]]; then
      as_root systemctl enable --now shaer-librespot.service
    fi
    if [[ "$ENABLE_BLE" == "1" ]]; then
      as_root systemctl enable --now shaer-ble-discovery.service
    fi
  fi
}

validate_platform
install_packages
ensure_user_and_dirs
install_services

cat <<DONE
SHAeR bootstrap complete.
Mode: $([[ "$MOCK_HARDWARE" == "1" ]] && echo mock-hardware || echo hardware-selected)
Start service later: sudo systemctl start shaer-pi-os.service
Health check: $OUTPUTS_DIR/shaer_pi_os/health_check.sh
Diagnostics: $OUTPUTS_DIR/shaer_pi_os/diagnostics.sh
DONE
