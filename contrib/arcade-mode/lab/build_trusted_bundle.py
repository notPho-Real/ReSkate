"""Stage a reviewable bundle inside the checkout. Never install or approve it."""
import hashlib
import json
from pathlib import Path
import sys


def build(output):
    here = Path(__file__).resolve().parent
    root = here.parents[2]
    output = Path(output).absolute()
    if output.exists() or (root / 'artifacts').resolve() not in output.resolve().parents:
        raise ValueError('Use a new directory below project artifacts')
    for path in (output, *output.parents):
        if path.is_symlink():
            raise ValueError('Unsafe staging path')
    output.mkdir(mode=0o700)
    batch = (here / 'trick_batch.py').read_text()
    files = {
        'trusted_runner.py': (here / 'trusted_runner.py').read_text(),
        'plan_schema.py': (here / 'plan_schema.py').read_text(),
        'worker_robot.py': (here / 'robot.py').read_text().split('\ndef remote_source(')[0],
        'worker_probe.py': (here / 'probe.py').read_text(),
        'worker_batch.py': 'import sys\n' + batch[batch.index('\ndef phase_events'):batch.index('# LOCAL TRANSPORT:')],
    }
    for name, value in files.items():
        compile(value, name, 'exec')
        (output / name).write_text(value)
    manifest = {'version': 4, 'repository': 'notPho-Real/ReSkate', 'host': 'deck@steamdeck.local',
                'pins': {}, 'files': {name: hashlib.sha256(value.encode()).hexdigest() for name, value in files.items()}}
    (output / 'trust.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Staged only. Review every file; populate independently verified target pins before external installation.')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: build_trusted_bundle.py artifacts/NEW_DIRECTORY')
    build(sys.argv[1])
