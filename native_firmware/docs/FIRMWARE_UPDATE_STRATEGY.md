# SHAeR Firmware Update Strategy V1

Status: Required before hardware testing.

## Update Flow

```text
Current firmware
-> Companion stages update
-> Verify SHA256
-> Backup current firmware + settings
-> Install inactive slot
-> Reboot
-> Health check
-> Mark boot successful
-> Keep new firmware
```

## Rollback Flow

```text
Reboot after update
-> crash loop or health check failure
-> Safe Mode
-> restore previous firmware slot
-> preserve logs
-> show recovery popup
```

## Required Metadata

| Field | Required |
|---|---|
| Firmware version | Yes |
| Compatible hardware revision | Yes |
| SHA256 | Yes |
| Required schema version | Yes |
| Rollback supported | Yes |
| Release notes | Yes |

## Health Check

An update is healthy only if:

- Settings migration succeeds.
- Boot reaches SHAeR OS.
- Display can render Home or Safe Mode.
- Event loop runs for at least 2 seconds.
- Logs can be written.

