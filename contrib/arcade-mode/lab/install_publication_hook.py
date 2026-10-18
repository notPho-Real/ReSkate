"""Install a reviewed external guard without overwriting any existing hooks."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess


WRAPPER = '''#!/usr/bin/python3 -I
import pathlib, subprocess, sys
here = pathlib.Path(__file__).resolve().parent
data = sys.stdin.buffer.read()
prior = here / 'prior-pre-push'
if prior.exists():
    result = subprocess.run([str(prior), *sys.argv[1:]], input=data)
    if result.returncode:
        sys.exit(result.returncode)
root = subprocess.run(['/usr/bin/git', 'rev-parse', '--show-toplevel'],
                      check=True, capture_output=True).stdout.decode().strip()
result = subprocess.run(['/usr/bin/python3', '-I', '-B', str(here / 'publication_guard.py'),
                         *sys.argv[1:], '--root', root,
                         '--review-file', str(here / 'review.json')], input=data)
sys.exit(result.returncode)
'''


def install(root, destination):
    root, destination = Path(root).resolve(), Path(destination).resolve()
    if destination == root or root in destination.parents:
        raise ValueError('Trusted hook directory must be outside the checkout')
    if destination.exists():
        raise ValueError('Destination must be new')
    def git(*args):
        return subprocess.run(['/usr/bin/git', '-C', str(root), *args], check=True,
                              capture_output=True).stdout.decode().strip()
    old = Path(git('rev-parse', '--git-path', 'hooks'))
    if not old.is_absolute():
        old = root / old
    if old.is_symlink() or any(p.is_symlink() for p in old.rglob('*')):
        raise ValueError('Symlinked hooks need separate review')
    if (old / 'prior-pre-push').exists():
        raise ValueError('Existing chain name requires separate review')
    before = {str(p.relative_to(old)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in old.rglob('*') if p.is_file()}
    # No existing byte is modified; config changes only after the complete copy.
    shutil.copytree(old, destination)
    destination.chmod(0o700)
    if (destination / 'pre-push').exists():
        (destination / 'pre-push').rename(destination / 'prior-pre-push')
    for name in ('publication_guard.py', 'naming_guard.py'):
        if (destination / name).exists():
            # Existing helper may be depended on by old hooks. Never replace it.
            if (destination / name).read_bytes() != Path(__file__).with_name(name).read_bytes():
                raise ValueError('Existing helper differs; integration needs separate review')
        else:
            shutil.copy2(Path(__file__).with_name(name), destination / name)
    (destination / 'pre-push').write_text(WRAPPER)
    (destination / 'pre-push').chmod(0o700)
    after = {str(p.relative_to(old)): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in old.rglob('*') if p.is_file()}
    if before != after:
        raise ValueError('Existing hooks changed during installation')
    for name, digest in before.items():
        copied = destination / ('prior-pre-push' if name == 'pre-push' else name)
        if hashlib.sha256(copied.read_bytes()).hexdigest() != digest:
            raise ValueError('Preserved hook copy differs')
    git('config', '--local', 'core.hooksPath', str(destination))
    return before


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    args = parser.parse_args()
    print('Preserved hook hashes:', install(args.root, args.destination))
