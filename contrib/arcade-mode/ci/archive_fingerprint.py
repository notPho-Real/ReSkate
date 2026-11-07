"""Fingerprint a COFF archive, excluding only documented timestamp fields.

No bytes used by the build are changed. Unknown object formats are hashed in
full. The archive layout, symbols, sections, code and all other fields remain.
See Microsoft's PE/COFF archive and object header specification.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys


def fingerprint(raw):
    if not raw.startswith(b"!<arch>\n"):
        raise ValueError("Expected a COFF archive")
    data = bytearray(raw)
    offset, members, timestamps = 8, 0, []
    while offset < len(data):
        if offset + 60 > len(data) or data[offset + 58:offset + 60] != b"\x60\n":
            raise ValueError("Malformed archive member header")
        header = bytes(data[offset:offset + 60])
        name = header[:16].rstrip()
        size = int(header[48:58].strip())
        if size < 0 or offset + 60 + size > len(data):
            raise ValueError("Truncated archive member")
        date = header[16:28].strip()
        if date and not date.isdigit():
            raise ValueError("Malformed archive timestamp")
        data[offset + 16:offset + 28] = b"0".ljust(12)
        timestamps.append([offset + 16, 12, "archive"])
        start = offset + 60
        obj = bytes(data[start:start + size])
        stamp = None
        if name not in (b"/", b"//", b"/SYM64/"):
            if len(obj) >= 20 and obj[:2] == b"\x64\x86":
                sections = struct.unpack_from("<H", obj, 2)[0]
                optional = struct.unpack_from("<H", obj, 16)[0]
                if optional == 0 and 20 + 40 * sections <= len(obj):
                    stamp = 4
            elif len(obj) >= 20 and obj[:4] == b"\x00\x00\xff\xff" and obj[6:8] == b"\x64\x86":
                version = struct.unpack_from("<H", obj, 4)[0]
                if version == 0 and 20 + struct.unpack_from("<I", obj, 12)[0] == len(obj):
                    stamp = 8  # short import object
                elif (version == 2 and len(obj) >= 56 and
                      obj[12:28] == bytes.fromhex("c7a1bad1eebaa94baf20faf66aa4dcb8")):
                    stamp = 8  # ANON_OBJECT_HEADER_BIGOBJ
        if stamp is not None:
            data[start + stamp:start + stamp + 4] = b"\0" * 4
            timestamps.append([start + stamp, 4, "coff"])
        offset = start + size + (size % 2)
        members += 1
    if offset != len(data):
        raise ValueError("Missing archive alignment")
    return dict(raw_sha256=hashlib.sha256(raw).hexdigest(),
                content_sha256=hashlib.sha256(data).hexdigest(),
                members=members, timestamp_fields=len(timestamps))


if __name__ == "__main__":
    print(json.dumps(fingerprint(Path(sys.argv[1]).read_bytes())))

