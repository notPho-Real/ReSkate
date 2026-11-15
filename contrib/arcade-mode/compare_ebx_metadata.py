"""Compare native array/boxed type metadata without a compiler or game process.

This is a metadata check, not a full EBX validator or rendering acceptance.
Inputs are private local files; the report contains hashes/counts, not assets.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct


def read_metadata(data):
    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] not in (b"EBXS", b"EBX\0"):
        raise ValueError("Expected RIFF EBX")
    if struct.unpack_from("<I", data, 4)[0] + 8 != len(data):
        raise ValueError("RIFF size mismatch")
    chunks, at = {}, 12
    while at < len(data):
        if len(data) - at < 8:
            raise ValueError("Truncated chunk header")
        tag = bytes(data[at:at + 4])
        size = struct.unpack_from("<I", data, at + 4)[0]
        end = at + 8 + size
        if tag in chunks or end + (size & 1) > len(data):
            raise ValueError("Duplicate or truncated chunk")
        chunks[tag] = data[at + 8:end]
        at = end + (size & 1)
    try:
        fixup, extra = chunks[b"EFIX"], chunks[b"EBXX"]
    except KeyError as error:
        raise ValueError("Missing native metadata chunk") from error
    if len(fixup) < 24 or len(extra) < 8:
        raise ValueError("Truncated native metadata")
    count = struct.unpack_from("<I", fixup, 16)[0]
    signature_at = 20 + count * 16
    if signature_at + 4 > len(fixup):
        raise ValueError("Truncated type identities")
    signatures = struct.unpack_from("<I", fixup, signature_at)[0]
    if signatures > count or signature_at + 4 + signatures * 4 > len(fixup):
        raise ValueError("Invalid signature table")
    identities = []
    for index in range(count):
        guid = fixup[20 + index * 16:36 + index * 16].hex()
        signature = struct.unpack_from("<I", fixup, signature_at + 4 + index * 4)[0] if index < signatures else None
        identities.append((guid, signature))
    arrays, boxes = struct.unpack_from("<II", extra)
    if len(extra) != 8 + 16 * (arrays + boxes):
        raise ValueError("Invalid extra-record table")
    records = []
    for index in range(arrays + boxes):
        _, length, group, encoded, slot = struct.unpack_from("<IIIHH", extra, 8 + index * 16)
        if slot != 0xffff and slot >= len(identities):
            raise ValueError("Type reference outside EFIX")
        if index < arrays and length > 0x7fffffff:
            raise ValueError("Negative array count")
        identity = ("primitive", None) if slot == 0xffff else identities[slot]
        records.append((length, group, encoded, identity))
    return {"sha256": hashlib.sha256(data).hexdigest(),
            "arrays": records[:arrays], "boxed": records[arrays:]}


def compare(before, after, allow_non_enum_array_edits=False):
    failures, checked = [], {}
    for kind in ("arrays", "boxed"):
        left, right = before[kind], after[kind]
        if kind == "arrays" and allow_non_enum_array_edits:
            # EBXX encodes the reflected field type shifted one additional bit.
            left = [row for row in left if (row[2] >> 5) & 0x1f == 8]
            right = [row for row in right if (row[2] >> 5) & 0x1f == 8]
        checked[kind] = {"before": len(left), "after": len(right)}
        if Counter(left) != Counter(right):
            failures.append(kind)
    return {"metadata_preserved": not failures, "mismatched_tables": failures,
            "checked_records": checked,
            "scope": "enum arrays and boxed records" if allow_non_enum_array_edits else "all array and boxed records",
            "before_sha256": before["sha256"], "after_sha256": after["sha256"],
            "native_rendering_verified": False}


def load(path):
    with path.open("rb") as stream:
        data = stream.read(16 * 1024 * 1024 + 1)
    if len(data) > 16 * 1024 * 1024:
        raise ValueError("Input exceeds bounded metadata-check size")
    return read_metadata(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--allow-non-enum-array-edits", action="store_true",
                        help="Check only enum arrays and boxed records; validate graph edits separately")
    args = parser.parse_args()
    try:
        report = compare(load(args.before), load(args.after), args.allow_non_enum_array_edits)
    except (OSError, ValueError, struct.error) as error:
        print(json.dumps({"metadata_preserved": False, "error": str(error)}))
        return 2
    print(json.dumps(report, indent=2))
    return 0 if report["metadata_preserved"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
