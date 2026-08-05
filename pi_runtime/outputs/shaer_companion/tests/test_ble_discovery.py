from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[2] / "shaer_pi_os" / "shaer_ble_discovery.py"
SPEC = importlib.util.spec_from_file_location("shaer_ble_discovery", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class BluetoothDiscoveryPayloadTests(unittest.TestCase):
    def test_endpoint_payload_is_version_ip_and_big_endian_port(self) -> None:
        self.assertEqual(MODULE.endpoint_payload("192.168.1.42", 8775), bytes.fromhex("01c0a8012a2247"))

    def test_ipv6_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            MODULE.endpoint_payload("::1", 8775)

    def test_invalid_port_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            MODULE.endpoint_payload("192.168.1.42", 0)


if __name__ == "__main__":
    unittest.main()
