# SHAeR security remediation checkpoint

This build closes the critical network and storage findings identified before hardware deployment.

## Enforced boundaries

- Device event polling, Spotify runtime routes, recording routes, metrics, and system actions are loopback-only.
- Pairing approval and shutdown require a short-lived, one-use capability produced by a physical OK/long-OK GPIO event.
- The old state-changing `GET /api/push` route returns 405. Bench input is an explicit `--allow-test-input` POST endpoint and is loopback-only.
- The production service no longer enables poweroff by default. Enable `--allow-power` only after validating the physical capability flow and sudoers rule on the target Pi.
- Companion routes remain bearer-token authenticated except discovery, pairing start, and pairing status.
- Static serving is allowlisted to theme assets, companion assets, and the two shared browser assets. Python source, diagnostics, docs, and service files are not web-accessible.
- Responses include CSP, clickjacking, MIME-sniffing, referrer, permissions, and resource-policy headers.

## Storage defenses

- Unsigned theme installation is disabled. The companion import control is removed.
- ZIP members are bounded by count, expanded size, path depth, name length, compression ratio, and file type. Links, special files, encrypted members, duplicates, and traversal paths are rejected.
- Audio uploads require both a supported extension and matching file signature.
- Playlist track IDs are validated before mutation.
- JSON read-modify-write operations hold one lock through the atomic replace.
- Restored SQLite databases pass a read-only integrity and schema check, then stage as `*.restore-pending` for activation on restart.
- Theme restoration is disabled; themes are restored by reinstalling a trusted device bundle.

## Remaining architecture work

This checkpoint does not claim completion of the larger one-renderer migration. The six themes still contain theme-specific browser applications. Moving them to a single state-driven renderer and wiring every settings control to a real subsystem remain separate implementation work and should happen after Raspberry Pi security and hardware acceptance tests pass.
