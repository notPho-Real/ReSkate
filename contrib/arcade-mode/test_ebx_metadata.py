"""Authored metadata fixtures; no game data, compiler or device required."""
import struct
import unittest
from compare_ebx_metadata import compare, read_metadata


def document(slot=0, reverse=False, copies=1, primitive=False, boxed=False):
    guids = [bytes([1]) * 16, bytes([2]) * 16]
    signatures = [7, 9]
    if reverse:
        guids.reverse()
        signatures.reverse()
    fixup = bytes(16) + struct.pack("<I", 2) + b"".join(guids)
    fixup += struct.pack("<III", 2, *signatures)
    extra = struct.pack("<II", 0 if boxed else copies, copies if boxed else 0)
    extra += struct.pack("<IIIHH", 0, 2, 1234, 0x1e8 if primitive else 0x108, slot) * copies
    def chunk(tag, body):
        return tag + struct.pack("<I", len(body)) + body + bytes(len(body) & 1)
    body = b"EBXS" + chunk(b"EFIX", fixup) + chunk(b"EBXX", extra)
    return b"RIFF" + struct.pack("<I", len(body)) + body


class MetadataTests(unittest.TestCase):
    def test_renumbering_preserves_identity(self):
        self.assertTrue(compare(read_metadata(document()), read_metadata(document(1, reverse=True)))["metadata_preserved"])

    def test_lost_enum_type_is_detected(self):
        before, after = read_metadata(document()), read_metadata(document(0xffff))
        for scoped in (False, True):
            self.assertFalse(compare(before, after, scoped)["metadata_preserved"])

    def test_stale_slot_and_duplicate_loss_are_detected(self):
        before = read_metadata(document())
        self.assertFalse(compare(before, read_metadata(document(reverse=True)))["metadata_preserved"])
        self.assertFalse(compare(before, read_metadata(document(copies=2)))["metadata_preserved"])

    def test_edit_scope_is_explicit(self):
        before = read_metadata(document(0xffff, primitive=True))
        after = read_metadata(document(0xffff, copies=2, primitive=True))
        self.assertFalse(compare(before, after)["metadata_preserved"])
        report = compare(before, after, True)
        self.assertTrue(report["metadata_preserved"])
        self.assertEqual(report["checked_records"]["arrays"]["before"], 0)
        self.assertFalse(report["native_rendering_verified"])

    def test_boxed_type_change_is_detected_in_both_scopes(self):
        before = read_metadata(document(boxed=True))
        after = read_metadata(document(1, boxed=True))
        for scoped in (False, True):
            report = compare(before, after, scoped)
            self.assertFalse(report["metadata_preserved"])
            self.assertEqual(report["mismatched_tables"], ["boxed"])

    def test_malformed_inputs_are_refused(self):
        for data in (document()[:-1], document(3), b"RIFF", document() + b"x"):
            with self.subTest(size=len(data)):
                with self.assertRaises(ValueError):
                    read_metadata(data)

    def test_duplicate_chunks_are_refused(self):
        data = bytearray(document())
        data += data[12:]
        struct.pack_into("<I", data, 4, len(data) - 8)
        with self.assertRaises(ValueError):
            read_metadata(data)


if __name__ == "__main__":
    unittest.main()
