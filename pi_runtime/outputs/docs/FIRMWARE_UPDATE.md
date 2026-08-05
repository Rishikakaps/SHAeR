# SHAeR Firmware Update Flow

The companion never installs files directly into the running firmware.

1. The client submits a manifest (`version`, `sha256`, signature) and base64 package.
2. SHAeR verifies the SHA-256 checksum.
3. When `SHAER_UPDATE_PUBLIC_KEY` is configured, SHAeR verifies the package with OpenSSL and that public key.
4. Only verified packages are atomically staged under `~/.config/shaer/updates`.
5. Installation and rollback call the root-owned executable configured as `SHAER_UPDATE_HELPER` with `install` or `rollback`.
6. The helper must preserve `/home`, the library database, settings, music, themes, and backups; install into an inactive release slot; update the boot pointer atomically; and reboot.
7. A watchdog must return to the previous slot if the new release does not report healthy.

Unsigned packages are accepted only when a manifest explicitly sets `development_unsigned=true` and no production key is configured. The UI labels this as development behavior.

The verifier and staging flow are implemented and tested locally. Privileged installation, rollback, reboot, and recovery-mode acceptance are **PENDING** on real Raspberry Pi hardware.

