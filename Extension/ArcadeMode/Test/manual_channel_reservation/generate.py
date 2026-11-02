"""Compile exact Manual channel publication and ground payload transport statements offline."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

VARIANTS = ("current", "rollback-owner", "rollback-reservation", "rollback-competitor", "rollback-rs", "rollback-release")


def once(text, marker):
    if text.count(marker) != 1:
        raise ValueError('ambiguous source marker: ' + marker)
    return text.index(marker)


def generate(root, variant="current"):
    if variant not in VARIANTS:
        raise ValueError("unknown control")
    mode = (root / 'Extension/ArcadeMode/arcade_mode.cpp').read_text()
    client = (root / 'Extension/Skater/client_noclip.cpp').read_text()
    parts = {}
    start = once(mode, '        const bool linked_manual_delivery = ')
    end = once(mode, '        const bool manual_delivery = ')
    parts['DELIVERY'] = mode[start:mode.index(';', end)+1]
    start = once(mode, '        // Manual vertical reservation publication begin.')
    end = once(mode, '        // Manual vertical reservation publication end.')
    parts['RESERVE'] = mode[start:end]
    for key, marker in [('STORE', '    a.ground_controls.store('),
                        ('LOAD', '    candidate.ground_controls = arcade_mode::GroundControl::decode(')]:
        start = once(client, marker)
        parts[key] = client[start:client.index(';', start)+1]
    source = (root / 'Extension/ArcadeMode/Test/manual_channel_reservation/fixture.cpp.in').read_text()
    for key, value in parts.items():
        once(source, '@@' + key + '@@')
        source = source.replace('@@' + key + '@@', value)
    if '@@' in source:
        raise ValueError('unresolved template')
    controls = {
        "rollback-owner": ("result.manual.owns && (result.manual.request == -1 || result.manual.request == 1)", "(result.manual.request == -1 || result.manual.request == 1)"),
        "rollback-reservation": ("ground_controls.manual_vertical_reserved = rotation_enabled && manual_delivery &&", "ground_controls.manual_vertical_reserved = false && rotation_enabled && manual_delivery &&"),
        "rollback-competitor": ("!result.rail.held && !rail_pad.owns_right_stick && !rail_pad.button_mask &&", "true &&"),
        "rollback-rs": ("result.right_x == 0 && result.right_y == result.manual.request * 20000", "true"),
        "rollback-release": ("(releasing_manual && result.right_x == 0 && result.right_y == 0)", "(false && releasing_manual && result.right_x == 0 && result.right_y == 0)"),
    }
    if variant != "current":
        old, new = controls[variant]
        once(source, old)
        source = source.replace(old, new)
    return source, {"variant": variant, "production_chunks_sha256": {
        key: hashlib.sha256(value.encode()).hexdigest() for key, value in parts.items()},
        "fixture_sha256": hashlib.sha256(source.encode()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--variant', choices=VARIANTS, default='current')
    parser.add_argument('--verify-control', choices=VARIANTS[1:])
    parser.add_argument('--binary', type=Path)
    args = parser.parse_args()
    if args.verify_control:
        if not args.binary:
            parser.error('--verify-control requires --binary')
        result = subprocess.run([str(args.binary.resolve())], capture_output=True, text=True, timeout=60)
        expected = 'only final actual Manual hold or owned neutral release reserves the vertical channel'
        if result.returncode != 1 or result.stderr.strip() != expected:
            raise ValueError('rollback failed outside intended assertion: ' + result.stderr)
        print('Manual channel rollback assertion verified: ' + args.verify_control)
        return
    if not args.root or not args.output:
        parser.error('--root and --output required')
    source, hashes = generate(args.root.resolve(), args.variant)
    if args.check:
        if args.output.read_text() != source:
            raise ValueError('stale source composition')
        print('Manual channel source correspondence verified')
    else:
        args.output.write_text(source)
        args.output.with_suffix('.json').write_text(json.dumps(hashes, indent=2) + '\n')


if __name__ == '__main__':
    main()
