import sys
from pathlib import Path
import unittest
from unittest.mock import MagicMock, Mock

sys.path.insert(0, str(Path(__file__).resolve().parent / "lab"))
import wake


class WakeTests(unittest.TestCase):
    def test_packet(self):
        packet = wake.magic_packet("02:11:22:33:44:55")
        self.assertEqual(len(packet), 102)
        self.assertEqual(packet, b"\xff" * 6 + bytes.fromhex("021122334455") * 16)

    def test_bad_mac(self):
        for mac in (None, "", "$(id)", "00:00:00:00:00:00", "ff:ff:ff:ff:ff:ff", "02:11:22:33:44"):
            with self.assertRaises(ValueError):
                wake.magic_packet(mac)

    def test_bounded_send_and_close(self):
        factory = MagicMock()
        connection = factory.return_value.__enter__.return_value
        connection.sendto.return_value = 102
        sleep = Mock()
        self.assertEqual(wake.send_wake("02:11:22:33:44:55", "192.168.50.255", factory, sleep), 3)
        self.assertEqual(connection.sendto.call_count, 3)
        self.assertEqual(sleep.call_count, 2)
        factory.return_value.__exit__.assert_called_once()

    def test_reject_external_before_socket(self):
        factory = Mock()
        for address in ("8.8.8.8", "127.0.0.1", "224.0.0.1", "0.0.0.0", "::1", "example.com"):
            with self.assertRaises(ValueError):
                wake.send_wake("02:11:22:33:44:55", address, factory)
        factory.assert_not_called()

    def test_send_failure_closes_socket(self):
        factory = MagicMock()
        factory.return_value.__enter__.return_value.sendto.return_value = 0
        with self.assertRaises(OSError):
            wake.send_wake("02:11:22:33:44:55", "192.168.50.255", factory)
        factory.return_value.__exit__.assert_called_once()


if __name__ == "__main__":
    unittest.main()
