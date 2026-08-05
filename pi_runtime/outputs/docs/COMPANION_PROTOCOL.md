# SHAeR Companion Protocol v1

The companion protocol is transport-independent JSON. Every response has one of these forms:

```json
{"version": 1, "ok": true, "data": {}}
```

```json
{"version": 1, "ok": false, "error": {"code": "invalid_token", "message": "..."}}
```

Wi-Fi currently carries JSON over HTTP. BLE uses the same JSON messages with the framing in `BLE_API.md`. USB is reserved for a future adapter. Business logic lives in `shaer_companion.protocol.CompanionService`; transports only parse, authenticate, and route messages.

## Pairing

1. `POST /api/v1/pairing/start` with `{"device_name":"My phone"}`.
2. Both screens show the returned six-digit code.
3. SHAeR displays the request as a system overlay. OK approves; Back denies.
4. The companion polls `GET /api/v1/pairing/status?pairing_id=...`.
5. After approval the token is delivered once. Store it in protected app storage.
6. Send `Authorization: Bearer <token>` on protected requests.

Tokens contain 384 bits of randomness. SHAeR stores only SHA-256 token digests in an owner-only file. Trusted devices can be listed and forgotten. A lost token requires pairing again.

## Endpoint groups

| Group | Operations |
| --- | --- |
| Device | discovery, dashboard, mirrored playback actions |
| Pairing | start, status, local approve/deny, trusted devices |
| Music | list/search/upload/update/delete tracks; playlist CRUD/import/export |
| Themes | list, activate, package import/export/delete |
| Settings | read and patch the complete settings tree |
| Diagnostics | list and run existing SHAeR diagnostic scripts |
| Updates | status, verify/stage, install, rollback |
| Backup | encrypted create and selective restore |

Unknown endpoints return `not_found`. Invalid credentials return HTTP 401. Device-only approval from a non-loopback client returns HTTP 403. Oversized JSON file transfers return HTTP 413.

## Compatibility

Clients must check `protocol_version` from discovery. Additive fields may appear in v1. Removing fields, changing field meaning, or changing authentication requires a new protocol version.

