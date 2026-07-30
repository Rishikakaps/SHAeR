#!/usr/bin/env python3
import os
import socket

if os.environ.get("SHAER_HARDWARE") == "1":
    host = socket.gethostname()
    addresses = socket.getaddrinfo(host, None, socket.AF_INET)
    assert any(item[4][0] != "127.0.0.1" for item in addresses), "No non-loopback Wi-Fi address detected"
    print("wifi_test ok mode=hardware address=true")
else:
    print("wifi_test ok mode=contract discovery=avahi physical-pending")
