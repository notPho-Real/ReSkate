"""The cache audit must never hide code, symbol, flag or layout differences."""
import struct
import unittest
from archive_fingerprint import fingerprint


def archive(stamp=123, payload=b"code"):
    obj = struct.pack("<HHIIIHH", 0x8664, 0, stamp, 0, 0, 0, 0) + payload
    header = b"sample.obj/".ljust(16) + str(stamp).encode().ljust(12)
    header += b"0".ljust(6) * 2 + b"644".ljust(8) + str(len(obj)).encode().ljust(10) + b"\x60\n"
    return b"!<arch>\n" + header + obj + (b"\n" if len(obj) % 2 else b"")


class ArchiveFingerprintTests(unittest.TestCase):
    def test_only_documented_timestamps_are_ignored(self):
        a, b = fingerprint(archive(123)), fingerprint(archive(456))
        self.assertNotEqual(a["raw_sha256"], b["raw_sha256"])
        self.assertEqual(a["content_sha256"], b["content_sha256"])
        self.assertEqual(a["timestamp_fields"], 2)

    def test_payload_flag_and_name_changes_remain_visible(self):
        a = archive()
        for offset in (8, 8 + 60 + 18, len(a) - 2):
            b = bytearray(a)
            b[offset] ^= 1
            self.assertNotEqual(fingerprint(a)["content_sha256"], fingerprint(b)["content_sha256"])
        self.assertNotEqual(fingerprint(a)["content_sha256"],
                            fingerprint(archive(payload=b"stale"))["content_sha256"])

    def test_unknown_objects_are_hashed_verbatim(self):
        a = bytearray(archive())
        a[68:70] = b"??"
        b = bytearray(a)
        b[72] ^= 1
        self.assertNotEqual(fingerprint(a)["content_sha256"], fingerprint(b)["content_sha256"])

    def test_malformed_archives_fail(self):
        for value in (b"", archive()[:-2], archive().replace(b"\x60\n", b"!!")):
            with self.assertRaises(ValueError):
                fingerprint(value)


if __name__ == "__main__":
    unittest.main()

