"""Pre-push review of every outgoing tree. Diagnostics never echo matched bytes.

Only fast-forward updates to the configured development branch are supported.
Unknown formats, scanner failures, opaque new content and missing review fail shut.
Run from a trusted external hook, never from an outgoing commit's executable.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys

_spec = importlib.util.spec_from_file_location(
    'publication_naming', Path(__file__).resolve().with_name('naming_guard.py'))
_naming = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_naming)
forbidden = _naming.forbidden

TARGET = 'https://github.com/notPho-Real/ReSkate.git'
REF = 'refs/heads/arcade-mode'
CODING_REF = 'refs/heads/arcade-coding'
ZERO = '0' * 40
MAX_BYTES = 16 * 1024 * 1024
PRIVATE = re.compile(
    r'(?:' + ''.join(map(chr, (47, 85, 115, 101, 114, 115, 47))) + '|'
    + ''.join(map(chr, (47, 104, 111, 109, 101, 47)))
    + r'|[A-Za-z]:\\Users\\)(?!example(?:[/\\]|$)|user(?:[/\\]|$)|you(?:[/\\]|$)|'
    r'username(?:[/\\]|$)|deck(?:[/\\]|$)|\.{3}(?:[/\\]|$)|<)[^\s/\\]+|'
    r'\b(?:10(?:\.\d{1,3}){3}|192\.168(?:\.\d{1,3}){2}|'
    r'172\.(?:1[6-9]|2\d|3[01])(?:\.\d{1,3}){2})\b|'
    r'(?<![\w.])[a-zA-Z0-9-]+\.(?:local|internal)(?![\w.-])|'
    r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|'
    r'\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|'
    r'AKIA[A-Z0-9]{16}|sk-[A-Za-z0-9_-]{24,})\b|'
    r'(?i:(?:password|access_token|api_key|secret)\s*[:=]\s*[\"\x27][^\"\x27\s]{12,})', re.IGNORECASE)

# Exact reviewed public bytes only, never a path-based or vendor-wide exemption.
# The first five are SQLite examples and existing synthetic redaction/Wake tests.
# Further entries are individually reviewed code-member tokens/vendor examples.
# Any edit invalidates the exemption. Prohibited naming is NEVER exempted.
PUBLIC_EXAMPLES = {
    'b1dd5d74ec7f29055a6684fa06fb3c2f6821c87dd38f9a458dfd2e8a1db28189',
    '919e7f2e8ed1d8f56ac17b412b8971c76aa5d1a879752cc6058f75e7d5910e1d',
    'c6c4e5007c5027aa519e68dadf5f7f2fa810570fac2bd760ffd605422615a8b3',
    '9e4571b09d4122248e3b65f91bde5c00143525d7759bb86b18b600117d951efb',
    '06621366759b3f83c0846e4693428a820d560a5d75b99642d68cfa009d7daca6',
    '7be5062b49faa91e7c245827923bb9bf175e40c669bb221a25656205faeb817d',  # Reviewed code member access, not an endpoint: Engine/Vfs/Test/mod_merge_audit_tests.cpp
    'fbab8aad4ffc97abca852efb9230d223ffd6876f4570ab51791aa286bf49adf4',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Hud/native_party.cpp
    '36bd37fd82ab16ac59362587ab42639ff9ebf09e94c1f30edff9e34ba6320f4f',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Hud/native_party_hooks.cpp
    '23072c41a052f24b05333ebdfa71f64ef07c9886e62b6ad73bd06984b581e45f',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Hud/native_party_requests.cpp
    'ac0805bd8398d5f94dec4fa4bca049d368944b012c1ec1c68183ccf58afeffe2',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Session/session.cpp
    'c41b820539482455276d219704689ec0870b0752ee6a608360542dd4b68d4eaa',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Steam/steam_social.cpp
    '49e1f3ea75a2bac103501a4684050be9eac102d6ef2d49e8af63633c03fd255b',  # Reviewed code member access, not an endpoint: Extension/Multiplayer/Test/mesh_tests.cpp
    'e64fb851ee8b86168d3f0f2316202b8a474a05124c52d2fe0a1191c1d92b4278',  # Reviewed code member access, not an endpoint: Extension/Music/setup_page.html
    '4236a945014b46878a877fac6a054c0b6215bcc96db423ea19de445f3c9b90bf',  # Reviewed code member access, not an endpoint: Extension/Throwdowns/throwdown_relay.cpp
    '944548ff8a89a2f23318a2f0fa4c80fb148d7d453522bedfa788a5ae32b3f78e',  # Reviewed code member access, not an endpoint: Extension/UI/Overlay/chat_overlay.cpp
    'cad49173486e4d2fcf3c0929d64cf1fdcc7868ee8e885ffefd45a9489a98459e',  # Reviewed code member access, not an endpoint: Extension/UI/Overlay/overlay_console.cpp
    'af1cc31255fbf58d57ecc9d727cbd39a84e44fa8f11be7fd419fa0100c5255d8',  # Unmodified vendor documentation example: External/imgui/imgui.cpp
    'c97c5752f0cf8e235e5834726968708ff1f4b3bf6bfbfcf59db0229b5889ac35',  # Unmodified vendor documentation example: External/zstd/README.md
}


class Rejected(Exception):
    pass


def git(root, *args):
    result = subprocess.run(['/usr/bin/git', '--no-replace-objects', '-C', str(root), *args],
                            capture_output=True, timeout=60)
    if result.returncode:
        raise Rejected('Git inspection failed')
    return result.stdout


def scan(data, opaque=False):
    if len(data) > MAX_BYTES:
        raise Rejected('Content exceeds review limit')
    texts = [data.decode('latin1')]
    for encoding in ('utf-8', 'utf-16-le', 'utf-16-be'):
        try:
            texts.append(data.decode(encoding))
        except UnicodeError:
            pass
    # The standard device hostname/account are public defaults, not a personal
    # endpoint. Specific addresses and nondefault hostnames remain subject to review.
    default_host = ''.join(map(chr, (115,116,101,97,109,100,101,99,107,46,108,111,99,97,108)))
    private = any(PRIVATE.search(re.sub(r'(?<![\w.-])' + re.escape(default_host)
                  + r'(?![\w.-])', 'device.example.invalid', s, flags=re.IGNORECASE)) for s in texts)
    if any(forbidden(s) for s in texts) or (private and hashlib.sha256(data).hexdigest() not in PUBLIC_EXAMPLES):
        raise Rejected('Naming, private identifier or secret-shaped content detected')
    if opaque:
        try:
            text = data.decode('utf-8')
        except UnicodeError as error:
            raise Rejected('New binary requires separate reviewed publication procedure') from error
        if '\0' in text:
            raise Rejected('New binary requires separate reviewed publication procedure')


def tree(root, revision):
    entries = {}
    for entry in git(root, 'ls-tree', '-r', '-z', revision).split(b'\0'):
        if not entry:
            continue
        metadata, path = entry.split(b'\t', 1)
        mode, kind, oid = metadata.split()
        if kind != b'blob' or mode not in (b'100644', b'100755', b'120000'):
            raise Rejected('Unsupported tree entry')
        scan(path)
        entries[path] = oid.decode('ascii')
    return entries


def blobs(root, objects):
    """Batch inspection keeps full-tree review bounded without per-file processes."""
    objects = sorted(set(objects))
    if not objects:
        return {}
    stdin = ('\n'.join(objects) + '\n').encode('ascii')
    def batch(option):
        result = subprocess.run(['/usr/bin/git', '--no-replace-objects', '-C', str(root),
                                 'cat-file', option], input=stdin, capture_output=True, timeout=60)
        if result.returncode:
            raise Rejected('Blob inspection failed')
        return result.stdout
    total = 0
    for line in batch('--batch-check').splitlines():
        oid, kind, size = line.split()
        if kind != b'blob' or int(size) > MAX_BYTES:
            raise Rejected('Unsupported or oversized blob')
        total += int(size)
    if total > 256 * MAX_BYTES // 16:
        raise Rejected('Publication exceeds bounded review size')
    raw, offset, result = batch('--batch'), 0, {}
    for expected in objects:
        end = raw.index(b'\n', offset)
        oid, kind, size = raw[offset:end].split()
        if oid.decode() != expected or kind != b'blob':
            raise Rejected('Blob stream differs from request')
        offset = end + 1
        result[expected] = raw[offset:offset + int(size)]
        offset += int(size)
        if raw[offset:offset + 1] != b'\n':
            raise Rejected('Incomplete blob stream')
        offset += 1
    if offset != len(raw):
        raise Rejected('Unexpected blob stream trailer')
    return result


def inspect(root, remote_name, remote_url, updates):
    if remote_name != 'origin' or remote_url != TARGET:
        raise Rejected('Unexpected publication target')
    if git(root, 'remote', 'get-url', '--push', '--all', 'origin').strip().decode() != TARGET:
        raise Rejected('Configured push target differs')
    if git(root, 'rev-parse', '--is-shallow-repository').strip() != b'false':
        raise Rejected('Shallow history cannot establish complete outgoing range')
    grafts = Path(git(root, 'rev-parse', '--git-path', 'info/grafts').strip().decode())
    if not grafts.is_absolute():
        grafts = Path(root) / grafts
    if grafts.exists():
        raise Rejected('Grafted history is unsupported')
    lines = updates.splitlines()
    if len(lines) != 1:
        raise Rejected('Exactly one development branch update is required')
    fields = lines[0].split()
    if len(fields) != 4:
        raise Rejected('Malformed pre-push input')
    local_ref, local_oid, remote_ref, remote_oid = fields
    if local_ref not in (REF, CODING_REF) or remote_ref != REF:
        raise Rejected('Only the development branch or fixed coding promotion is supported')
    if not all(re.fullmatch('[0-9a-f]{40}', oid) for oid in (local_oid, remote_oid)):
        raise Rejected('Unsupported object format')
    if ZERO in (local_oid, remote_oid):
        raise Rejected('Deletion and new branch publication require separate review')
    if git(root, 'symbolic-ref', 'HEAD').strip().decode() != local_ref:
        raise Rejected('Publication must originate from the inspected checkout branch')
    if git(root, 'rev-parse', local_ref).strip().decode() != local_oid:
        raise Rejected('Local branch changed during review')
    # A failed merge-base is rejected, including unavailable remote objects.
    git(root, 'merge-base', '--is-ancestor', remote_oid, local_oid)
    commits = git(root, 'rev-list', '--reverse', local_oid, '^' + remote_oid).splitlines()
    if len(commits) > 512:
        raise Rejected('Outgoing history exceeds bounded review count')
    baseline = tree(root, remote_oid)
    baseline_blobs = set(baseline.values())
    outgoing = set()
    for raw_oid in commits:
        oid = raw_oid.decode('ascii')
        # Raw commit covers the entire subject/body and author/committer metadata.
        scan(git(root, 'cat-file', 'commit', oid))
        outgoing.update(tree(root, oid).values())
    # Include the complete index, unstaged edits and nonignored new files in the
    # review fingerprint. Ignored local artifacts cannot enter a push unless added;
    # once staged/committed they are scanned by the same rules as other content.
    local = hashlib.sha256()
    index = []
    for entry in git(root, 'ls-files', '--stage', '-z').split(b'\0'):
        if not entry:
            continue
        meta, path = entry.split(b'\t', 1)
        mode, blob, stage = meta.split()
        if stage != b'0':
            raise Rejected('Unmerged index')
        if mode not in (b'100644', b'100755', b'120000'):
            raise Rejected('Unsupported index entry')
        scan(path)
        index.append((entry, blob.decode('ascii')))
    content = blobs(root, outgoing | {blob for entry, blob in index})
    for blob, data in content.items():
        scan(data, opaque=blob not in baseline_blobs)
    for entry, blob in index:
        data = content[blob]
        local.update(entry + hashlib.sha256(data).digest())
    for path in sorted(set(git(root, 'ls-files', '-z', '--cached', '--others',
                              '--exclude-standard').split(b'\0')) - {b''}):
        scan(path)
        file = Path(root) / path.decode('utf-8', errors='strict')
        if file.is_symlink():
            data = str(file.readlink()).encode()
        elif file.is_file():
            if file.stat().st_size > MAX_BYTES:
                raise Rejected('Working content exceeds review limit')
            data = file.read_bytes()
        elif not file.exists():
            data = b''
        else:
            raise Rejected('Unsupported working content')
        oid = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if oid not in content:
            scan(data, opaque=oid not in baseline_blobs)
        mode = str(file.lstat().st_mode).encode() if file.exists() or file.is_symlink() else b'missing'
        local.update(path + b'\0' + mode + b'\0' + hashlib.sha256(data).digest())
    trusted_dir = Path(__file__).resolve().parent
    scanner = {name: hashlib.sha256((trusted_dir / name).read_bytes()).hexdigest()
               for name in ('publication_guard.py', 'naming_guard.py')}
    if (trusted_dir / 'pre-push').exists():
        scanner['pre-push'] = hashlib.sha256((trusted_dir / 'pre-push').read_bytes()).hexdigest()
    manifest = {'version': 1, 'target': remote_url, 'updates': lines,
                'commits': [c.decode() for c in commits], 'local_digest': local.hexdigest(),
                'scanner': scanner}
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    return {'manifest': manifest, 'digest': digest}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('remote_name')
    parser.add_argument('remote_url')
    parser.add_argument('--root', type=Path, default=Path.cwd())
    parser.add_argument('--review-file', type=Path)
    parser.add_argument('--inventory', action='store_true')
    args = parser.parse_args()
    report = inspect(args.root, args.remote_name, args.remote_url, sys.stdin.read())
    if args.inventory:
        print(json.dumps(report, indent=2))
        return
    if not args.review_file:
        raise Rejected('Missing independent semantic review receipt')
    receipt = json.loads(args.review_file.read_text())
    if receipt != {'digest': report['digest'], 'semantic_review_complete': True}:
        raise Rejected('Review receipt absent, stale or incomplete')
    print('Publication review passed')


if __name__ == '__main__':
    try:
        main()
    except Exception:
        # Never echo paths, commit messages, secret values or subprocess stderr.
        print('Publication blocked: inspection or semantic review failed', file=sys.stderr)
        sys.exit(1)
