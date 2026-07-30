#!/usr/bin/env python3
"""Advertise SHAeR's LAN endpoint in a compact Bluetooth LE packet."""

from __future__ import annotations

import argparse
import ipaddress
import socket
import subprocess
import time


MANUFACTURER_ID = 0x5348
PAYLOAD_VERSION = 1


def lan_ipv4() -> str | None:
    """Return the preferred non-loopback IPv4 address without external traffic."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("192.0.2.1", 9))
        candidate = sock.getsockname()[0]
    except OSError:
        candidate = None
    finally:
        sock.close()
    if candidate and not ipaddress.ip_address(candidate).is_loopback:
        return candidate
    try:
        for candidate in socket.gethostbyname_ex(socket.gethostname())[2]:
            if not ipaddress.ip_address(candidate).is_loopback:
                return candidate
    except OSError:
        pass
    return None


def endpoint_payload(address: str, port: int) -> bytes:
    ip = ipaddress.ip_address(address)
    if ip.version != 4:
        raise ValueError("SHAeR BLE discovery currently requires an IPv4 LAN address.")
    if not 1 <= port <= 65535:
        raise ValueError("Port must be between 1 and 65535.")
    return bytes([PAYLOAD_VERSION, *ip.packed, (port >> 8) & 0xFF, port & 0xFF])


def bluetoothctl_process(payload: bytes) -> subprocess.Popen[str]:
    process = subprocess.Popen(
        ["bluetoothctl"],
        stdin=subprocess.PIPE,
        text=True,
    )
    if process.stdin is None:
        raise RuntimeError("bluetoothctl did not provide an input stream.")
    data = " ".join(f"0x{byte:02x}" for byte in payload)
    commands = [
        "power on",
        "menu advertise",
        "clear manufacturer",
        f"manufacturer 0x{MANUFACTURER_ID:04x} {data}",
        "name SHAeR",
        "discoverable on",
        "back",
        "advertise on",
    ]
    process.stdin.write("\n".join(commands) + "\n")
    process.stdin.flush()
    return process


def run(port: int, refresh_s: float) -> None:
    process: subprocess.Popen[str] | None = None
    advertised: str | None = None
    try:
        while True:
            address = lan_ipv4()
            if address and (address != advertised or process is None or process.poll() is not None):
                if process is not None and process.poll() is None:
                    process.terminate()
                    process.wait(timeout=3)
                process = bluetoothctl_process(endpoint_payload(address, port))
                advertised = address
            time.sleep(refresh_s)
    finally:
        if process is not None and process.poll() is None:
            process.terminate()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8775)
    parser.add_argument("--refresh-seconds", type=float, default=10.0)
    parser.add_argument("--print-payload", metavar="IP", help="Print the BLE payload for diagnostics and exit.")
    args = parser.parse_args()
    if args.print_payload:
        print(endpoint_payload(args.print_payload, args.port).hex())
        return
    run(args.port, max(args.refresh_seconds, 2.0))


if __name__ == "__main__":
    main()
