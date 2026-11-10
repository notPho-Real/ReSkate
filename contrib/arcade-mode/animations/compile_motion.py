#!/usr/bin/env python3
"""Validate authored scalar keyframes and emit a deterministic C++ data table."""
import argparse
import json
import math
from pathlib import Path
import re

def compile_motion(source):
    data = json.loads(source)
    if set(data) != {"version", "namespace", "channels", "phases"} or data["version"] != 1:
        raise ValueError("unsupported motion schema")
    identifier = r"[a-zA-Z_][a-zA-Z0-9_]*"
    if not re.fullmatch(identifier + r"(?:::" + identifier + r")*", data["namespace"]):
        raise ValueError("invalid namespace")
    channels, phases = data["channels"], data["phases"]
    if not isinstance(channels, list) or not 1 <= len(channels) <= 32 or channels[0] != "time" or len(set(channels)) != len(channels):
        raise ValueError("invalid channels")
    if any(not re.fullmatch(identifier, c) for c in channels) or not isinstance(phases, dict) or not 1 <= len(phases) <= 16:
        raise ValueError("invalid phase layout")
    out = ["// Generated from authored motion JSON by compile_motion.py; edit the source data.", "#pragma once",
           "#include <array>", "namespace " + data["namespace"] + " {",
           "struct Key { float " + ", ".join(channels) + "; };"]
    for name, keys in phases.items():
        if not re.fullmatch(identifier, name) or not isinstance(keys, list) or not 1 <= len(keys) <= 200:
            raise ValueError("invalid phase")
        out.append(f"inline constexpr std::array<Key,{len(keys)}> {name}{{{{")
        previous = -1.0
        for key in keys:
            if not isinstance(key, list) or len(key) != len(channels) or any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 10000 for v in key):
                raise ValueError("invalid key")
            if key[0] <= previous or not 0 <= key[0] <= 60:
                raise ValueError("unordered or unbounded key time")
            previous = key[0]
            for channel in ("weight", "contact"):
                if channel in channels and not 0 <= key[channels.index(channel)] <= 1:
                    raise ValueError("invalid blend weight")
            values = []
            for value in key:
                text = format(float(value), ".8g")
                if "." not in text and "e" not in text: text += ".0"
                values.append(text + "f")
            out.append("    {" + ",".join(values) + "},")
        out.append("}};")
    out.append("}")
    return "\n".join(out) + "\n"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = compile_motion(args.source.read_text())
    if args.check:
        if args.output.read_text() != generated:
            raise SystemExit("motion data changed: regenerate the checked-in table")
    else:
        args.output.write_text(generated)
