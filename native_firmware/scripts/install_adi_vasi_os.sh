#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "Run as root on Raspberry Pi OS Lite: sudo scripts/install_adi_vasi_os.sh"
  exit 1
fi

if ! id shaer >/dev/null 2>&1; then
  useradd --system --home /var/lib/shaer --shell /usr/sbin/nologin shaer
fi
usermod -aG audio,bluetooth,gpio,i2c,spi shaer 2>/dev/null || true

install -d -o root -g root /opt/shaer
install -d -o shaer -g shaer \
  /var/lib/shaer \
  /var/lib/shaer/music \
  /var/lib/shaer/themes \
  /var/lib/shaer/cache \
  /var/lib/shaer/cache/artwork \
  /var/lib/shaer/db \
  /var/lib/shaer/updates \
  /var/lib/shaer/backups \
  /var/log/shaer

rsync -a --delete \
  --exclude '.git' \
  --exclude 'build/*' \
  --exclude 'releases/*' \
  --exclude '.companion_data' \
  --exclude 'work' \
  ./ /opt/shaer/
install -d -o root -g root /opt/shaer/build
install -m 0755 build/shaer_pi_bringup /opt/shaer/build/shaer_pi_bringup
if [ -f build/shaer_diag ]; then
  install -m 0755 build/shaer_diag /opt/shaer/build/shaer_diag
fi
chown -R root:root /opt/shaer
chown -R shaer:shaer /var/lib/shaer /var/log/shaer

install -m 0644 systemd/shaer.service /etc/systemd/system/shaer.service
install -m 0644 systemd/shaer.service /etc/systemd/system/shaer-app.service

# Hide Linux from the user-facing console. SHAeR owns tty1/framebuffer.
systemctl set-default multi-user.target
systemctl disable --now getty@tty1.service 2>/dev/null || true
systemctl disable --now lightdm.service 2>/dev/null || true
systemctl disable --now gdm.service 2>/dev/null || true
systemctl disable --now sddm.service 2>/dev/null || true
systemctl disable --now plymouth.service 2>/dev/null || true

if [ -f /boot/firmware/cmdline.txt ]; then
  CMDLINE=/boot/firmware/cmdline.txt
else
  CMDLINE=/boot/cmdline.txt
fi

if ! grep -q "quiet" "$CMDLINE"; then
  sed -i 's/$/ quiet loglevel=0 logo.nologo vt.global_cursor_default=0 consoleblank=0/' "$CMDLINE"
fi

systemctl daemon-reload
systemctl enable shaer.service

echo "आदि Vasi OS installed. Reboot to enter the SHAeR-owned foreground."

if [ "${SHAER_SKIP_REBOOT:-0}" = "0" ] && [ "${SHAER_AUTO_REBOOT:-0}" = "1" ]; then
  reboot
fi
