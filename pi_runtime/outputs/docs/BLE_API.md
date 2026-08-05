# SHAeR BLE API

BLE is an adapter for Companion Protocol v1; it must not add BLE-specific business behavior.

Proposed service UUID: `8f2d0001-53a3-4552-0000-534841455200`.

| Characteristic | UUID suffix | Direction | Purpose |
| --- | --- | --- | --- |
| Control | `0002` | write | JSON requests and request fragments |
| Response | `0003` | notify | JSON responses and response fragments |
| Status | `0004` | read/notify | protocol version, pairing and connection state |

Frames contain a one-byte version, one-byte flags, a two-byte big-endian message ID, a two-byte fragment index, and payload bytes. Flags mark first, final, response, and error fragments. Reassembled payloads are UTF-8 JSON identical to Wi-Fi request bodies.

Large music, theme, backup, and firmware transfers should negotiate Wi-Fi and are rejected over BLE with `transport_limit`. BLE is intended for discovery, pairing, settings, dashboard, and small commands.

Implementation and physical BLE acceptance are **PENDING** because no Raspberry Pi Bluetooth interface or phone is connected in this workspace.

