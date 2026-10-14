"""Fail closed on prohibited project naming in the index and commit message."""
import argparse
from pathlib import Path
import re
import subprocess
import sys

# Integer spelling keeps the prohibited strings themselves out of the tree.
WORDS = ((116, 104, 112, 115), (116, 104, 117, 103),
         (116, 111, 110, 121, 32, 104, 97, 119, 107),
         (110, 101, 118, 101, 114, 115, 111, 102, 116),
         (97, 99, 116, 105, 118, 105, 115, 105, 111, 110))
PATTERN = '|'.join(''.join(chr(c) for c in word).replace(' ', '[ _-]+') for word in WORDS)


def forbidden(text):
    return re.search(PATTERN, text, re.IGNORECASE) is not None


def check(root, message=None, cached=True):
    def git(*args):
        return subprocess.run(['/usr/bin/git', '-C', str(root), *args], capture_output=True, check=True).stdout
    paths = git('ls-files', '-z').decode().split('\0')
    problems = [p for p in paths if forbidden(p)]
    # Search all tracked bytes, including binary content, without printing them.
    command = ['/usr/bin/git', '-C', str(root), 'grep', '-a', '-l', '-i', '-E']
    if cached:
        command.append('--cached')
    result = subprocess.run(command + [PATTERN, '--'], capture_output=True)
    if result.returncode not in (0, 1):
        raise RuntimeError('Tracked-tree scan failed')
    problems.extend(result.stdout.decode().splitlines())
    branch = git('branch', '--show-current').decode()
    if forbidden(branch):
        problems.append('current branch')
    if message is not None and forbidden(message):
        problems.append('commit message')
    if problems:
        raise ValueError('Prohibited naming in: ' + ', '.join(sorted(set(problems))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.cwd())
    parser.add_argument('--message-file', type=Path)
    parser.add_argument('--working-tree', action='store_true')
    args = parser.parse_args()
    check(args.root, args.message_file.read_text() if args.message_file else None, not args.working_tree)
    print('Naming guard passed')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
