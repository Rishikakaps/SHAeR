# Native Android Discovery

## Verified protocol

SHAeR exposes plain HTTP on TCP port `8775`. The identification route is:

```text
GET /api/v1/device/discovery
```

The response is a versioned envelope whose `data` object includes `device_name`,
`firmware_version`, `protocol_version`, and pairing availability. The Pi service
binds to `0.0.0.0` in its systemd unit. The current server advertises `_shaer._tcp`
through Avahi on port `8775`, with `protocol=1` and `path=/companion` TXT records.

## Android architecture

The Android build is now a native Java bootstrap app. It probes the remembered
numeric address first, uses Android NSD/mDNS with a multicast lock, and falls
back to a bounded scan of the active IPv4 subnet. Every candidate is verified
against the identification route before it is displayed. Devices are deduplicated
by the returned device identity, and the last successful address is stored in
`SharedPreferences` for quick reconnection. After verification, the existing
companion web UI opens against the verified numeric address.

Manual host and port entry remains under Advanced connection only.

## Cleartext HTTP

The device protocol is intentionally local HTTP, not HTTPS. Android cleartext is
enabled for this app because the address is dynamic and may be any private LAN
IPv4 address. Discovery does not send credentials; pairing and authenticated API
requests remain governed by the existing SHAeR companion protocol. Public release
should move this transport to authenticated encryption before internet exposure.

## Build

```bash
npm run android:apk
```
