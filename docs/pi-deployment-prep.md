# SHAeR Pi Deployment Preparation

This pass prepares repeatable installation only. It does not validate GPIO, display rendering, DAC audio, battery behavior, Spotify playback, or microphone hardware.

## Configuration

Use `pi_runtime/outputs/shaer_pi_os/.env.example` as the source for `/etc/shaer/shaer.env`.

- `SHAER_USER`, `SHAER_GROUP`: service account.
- `SHAER_INSTALL_DIR`, `SHAER_OUTPUTS_DIR`: installed application location.
- `SHAER_CONFIG_DIR`, `SHAER_DATA_DIR`, `SHAER_CACHE_DIR`, `SHAER_LOG_DIR`: runtime state locations.
- `SHAER_HOST`, `SHAER_PORT`, `SHAER_THEME`: local HTTP server settings.
- `SHAER_MOCK_HARDWARE`: safe first-boot mode.
- `SHAER_ENABLE_GPIO`, `SHAER_ENABLE_DISPLAY`, `SHAER_ENABLE_AUDIO`, `SHAER_ENABLE_POWER`, `SHAER_ENABLE_BLE`: explicit integration gates.
- `SHAER_ALLOW_TEST_INPUT`: loopback-only bench input endpoint.
- `SHAER_PIN_A`, `SHAER_PIN_B`, `SHAER_PIN_OK`, `SHAER_PIN_BACK`, `SHAER_PIN_HOME`: BCM GPIO pins.
- `SHAER_ALSA_DEVICE`, `SHAER_MIC_DEVICE`: audio devices after hardware discovery.
- `SHAER_RECORDINGS_DIR`, `SHAER_RECORDINGS_DB`, `SHAER_RECORDING_MAX_SECONDS`, `SHAER_RECORDING_MIN_FREE_BYTES`: recorder storage and limits.
- `SPOTIFY_CLIENT_ID`, `SPOTIFY_REDIRECT_URI`: Spotify PKCE settings. No client secret is used or committed.

## Commands

Clean mock-mode install on Raspberry Pi OS Bookworm 64-bit:

```bash
sudo ./pi_runtime/outputs/shaer_pi_os/bootstrap_bookworm.sh --mock-hardware --enable-service
```

Laptop/static dry-run:

```bash
./pi_runtime/outputs/shaer_pi_os/bootstrap_bookworm.sh --dry-run --assume-bookworm --mock-hardware --skip-apt --install-dir /tmp/shaer --user "$USER" --systemd-dir /tmp/shaer-systemd
```

Health check:

```bash
./pi_runtime/outputs/shaer_pi_os/health_check.sh
```

Diagnostics:

```bash
./pi_runtime/outputs/shaer_pi_os/diagnostics.sh
```

Logs:

```bash
./pi_runtime/outputs/shaer_pi_os/logs.sh -f
```

Uninstall while preserving data:

```bash
sudo ./pi_runtime/outputs/shaer_pi_os/uninstall.sh
```

Rollback to a preserved previous install:

```bash
sudo SHAER_INSTALL_DIR=/opt/shaer ./pi_runtime/outputs/shaer_pi_os/rollback.sh /opt/shaer.previous
```

## Dependency Notes

Application JavaScript dependencies are locked by `package-lock.json` files. Python runtime dependencies are from the Raspberry Pi OS Bookworm apt repository where practical. OS packages are intentionally documented rather than fake-pinned, because Raspberry Pi OS security updates and mirror availability change over time.
