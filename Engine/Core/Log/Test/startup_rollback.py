"""Generate/check the one startup partial-record rollback; no live IO."""
import argparse
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--header', type=Path)
parser.add_argument('--output', type=Path)
parser.add_argument('--binary', type=Path)
parser.add_argument('--fixture', type=Path)
args = parser.parse_args()
if args.binary:
    if not args.fixture:
        parser.error('--binary requires --fixture')
    result = subprocess.run([str(args.binary.resolve()), str(args.fixture.resolve())],
                            capture_output=True, text=True, timeout=60)
    expected = 'startup rotation must reject a partial active record before moving or evicting evidence'
    if result.returncode != 1 or expected not in result.stderr.splitlines():
        print(result.stdout + result.stderr, file=sys.stderr)
        raise SystemExit('Startup rollback did not fail at its intended assertion')
    print('Startup partial-record rollback failed at the intended assertion')
else:
    if not args.header or not args.output:
        parser.error('generation requires --header and --output')
    text = args.header.read_text()
    first = '        // Startup rotation must reject an interrupted active record before it'
    last = '\n    }\n    void rotate()'
    if text.count(first) != 1 or text.count(last) != 1:
        raise SystemExit('Startup rollback source boundary changed; review required')
    start, end = text.index(first), text.index(last)
    if end <= start:
        raise SystemExit('Invalid startup rollback source boundary')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text[:start] + text[end:])
