"""Compile the actual optional observer and existing verified writer offline."""
import argparse
import hashlib
import json
from pathlib import Path


def between(source, first, last):
    if source.count(first) != 1 or source.count(last) != 1:
        raise ValueError('ambiguous source boundary')
    return source[source.index(first):source.index(last)]


def generate(root):
    mode = (root / 'Extension/ArcadeMode/arcade_mode.cpp').read_text()
    client = (root / 'Extension/Skater/client_noclip.cpp').read_text()
    chunks = {
        'OBSERVER': between(mode, 'GroundWriterEvidence begin_ground_writer_observation(', 'bool set_input_capture('),
        'ANGULAR_OBSERVER': between(mode, '// Angular writer observation begin. Every field', '// Angular writer observation end.'),
        'ANGULAR_BOUNDARIES': between(mode, '    // Angular receipt boundaries are separate', '    // Angular receipt boundaries end.'),
        'BODY_WRITE': between(client, 'template<class T> void body_write(', '\n}\nvoid debug_stop_noclip'),
        'FOLLOWUP_PUBLICATION': between(client, '    a.native_launch_id.store(', '    a.launch.store('),
        'COMMIT': between(client, '        const auto commit_motion = ', '        const auto release_turn = '),
        'ANGULAR_RELEASE': between(client, '        const auto release_turn = ', '        if (!intent.enabled) {'),
        'ANGULAR_BODY': between(client, '        auto applied=a.ground_output.ready', '\n    } catch (...) { a.controller.reset();'),
    }
    source = (root / 'Extension/ArcadeMode/Test/ground_writer/fixture.cpp.in').read_text()
    for name, text in chunks.items():
        marker = '@' + name + '@'
        if source.count(marker) != 1:
            raise ValueError('missing fixture marker')
        source = source.replace(marker, text)
    return source, {name: hashlib.sha256(text.encode()).hexdigest() for name, text in chunks.items()}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    source, hashes = generate(a.root)
    if a.check:
        if a.output.read_text() != source:
            raise ValueError('stale source composition')
        print('Ground writer source correspondence verified')
    else:
        a.output.write_text(source)
        a.output.with_suffix('.json').write_text(json.dumps(hashes, indent=2) + '\n')


if __name__ == '__main__':
    main()
