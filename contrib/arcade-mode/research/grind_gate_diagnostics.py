"""Bounded Python failure evidence for separately reviewed native experiments.

This module performs no memory reads, input, debugger calls or file I/O. It
serializes only selected values already read by the failing gate. Captured
locals are sequential observations, not an atomic native snapshot.
"""
import math

_FIELDS = ("state", "contact", "named", "output", "active", "actor", "inputs",
           "heading", "pitch", "roll", "native", "published", "angles",
           "points", "ends", "observed_segment", "segment_reference", "leg",
           "first_named", "second_named", "third_ready", "seen_contact")


def _bounded_value(value, depth=0):
    if value is None or type(value) is bool:
        return value
    if type(value) is int:
        return value if value.bit_length() <= 64 else "integer out of range"
    if type(value) is float:
        return value if math.isfinite(value) else "nonfinite"
    if type(value) is str:
        return value[:256]
    if type(value) is bytes:
        return {"hex": value[:96].hex(), "length": len(value)}
    if type(value) in (tuple, list) and depth < 3:
        return [_bounded_value(item, depth + 1) for item in value[:12]]
    # Never invoke game objects, debugger values, user repr or descriptors.
    return "not captured"


def gate_failure(exc, boundary, context):
    frames = []
    trace = exc.__traceback__
    visited = 0
    while trace is not None and visited < 32:
        frame = trace.tb_frame
        local = frame.f_locals
        frames.append({"function": frame.f_code.co_name[:80], "line": trace.tb_lineno,
                       "observed": {key: _bounded_value(local[key]) for key in _FIELDS if key in local}})
        trace = trace.tb_next
        visited += 1
    return {"boundary": boundary[:80], "exception": type(exc).__name__[:80],
            "message": _bounded_value(exc.args[0]) if exc.args else None,
            "frames": frames[-8:], "trace_truncated": trace is not None or visited > 8,
            "context": {key: _bounded_value(context[key]) for key in _FIELDS if key in context},
            "new_native_reads": False, "atomic_frame": False}
