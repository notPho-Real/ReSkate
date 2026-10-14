"""Send three Wake-on-LAN packets. Does not install, deploy or change Deck settings."""
import argparse
import ipaddress
import json
from pathlib import Path
import re
import socket
import time

CONFIG = Path(__file__).resolve().parents[3] / "tools/arcade-deck/wake.json"


def magic_packet(mac):
    if not isinstance(mac, str) or not re.fullmatch(r"(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}", mac):
        raise ValueError("Expected six colon-separated MAC bytes")
    address = bytes.fromhex(mac.replace(":", ""))
    if address == bytes(6) or address[0] & 1:
        raise ValueError("Expected a unicast device MAC")
    return b"\xff" * 6 + address * 16


def send_wake(mac, broadcast, factory=socket.socket, sleep=time.sleep):
    packet = magic_packet(mac)
    address = ipaddress.IPv4Address(broadcast)
    if not address.is_private or address.is_loopback or address.is_multicast or address.is_unspecified:
        raise ValueError("Use the local private subnet broadcast address")
    with factory(socket.AF_INET, socket.SOCK_DGRAM) as connection:
        connection.settimeout(2)
        connection.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        for attempt in range(3):
            if connection.sendto(packet, (str(address), 9)) != len(packet):
                raise OSError("Incomplete wake packet send")
            if attempt < 2:
                sleep(0.2)
    return 3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mac")
    parser.add_argument("--broadcast")
    args = parser.parse_args()
    settings = json.loads(CONFIG.read_text()) if CONFIG.is_file() else {}
    mac = args.mac or settings.get("mac")
    broadcast = args.broadcast or settings.get("broadcast")
    if not mac or not broadcast:
        parser.error("Supply --mac and --broadcast, or private tools/arcade-deck/wake.json")
    send_wake(mac, broadcast)
    print("Sent 3 wake packets. Wake is NOT confirmed; check SSH separately.")


if __name__ == "__main__":
    main()
