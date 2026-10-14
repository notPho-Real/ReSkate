"""Reviewed external HIL entry point. Install separately; stdin contains only data."""
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import stat
import subprocess
import sys
import time

VERSION = 13
MAX_REQUEST = 65536
ROOT = Path(__file__).resolve().parent
FILES = ('trusted_runner.py', 'plan_schema.py', 'worker_robot.py', 'worker_probe.py', 'worker_batch.py')
SETUP_ACTIONS = ('mount-board', 'pause', 'back', 'confirm', 'up', 'down', 'left', 'right',
                 'tab-left', 'tab-right', 'subtab-left', 'subtab-right')


def owned_file(path, limit):
    for parent in (path, *path.parents):
        if parent.is_symlink():
            raise ValueError('Symlink in trusted path')
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_uid != os.getuid() or info.st_mode & 0o022 or info.st_size > limit:
            raise ValueError('Unsafe trusted file')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            return stream.read(limit + 1)
    finally:
        os.close(fd)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate JSON field')
        result[key] = value
    return result


def decode(data):
    return json.loads(data, object_pairs_hook=unique_object,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('Nonfinite JSON')))


def load_bundle():
    trust = decode(owned_file(ROOT / 'trust.json', MAX_REQUEST))
    if set(trust) != {'version', 'files', 'host', 'repository', 'pins'} or trust['version'] != VERSION:
        raise ValueError('Invalid trust manifest')
    if trust['repository'] != 'notPho-Real/ReSkate' or trust['host'] != 'deck@steamdeck.local':
        raise ValueError('Target outside reviewed scope')
    if set(trust['files']) != set(FILES):
        raise ValueError('Invalid bundle inventory')
    sources = {}
    for name in FILES:
        data = owned_file(ROOT / name, 256 * 1024)
        if hashlib.sha256(data).hexdigest() != trust['files'][name]:
            raise ValueError('Trusted bundle digest mismatch; explicit review required')
        sources[name] = data.decode('utf-8')
    return trust, sources


def validate_request(request, trust, validate_plan):
    if not isinstance(request, dict) or set(request) != {'operation', 'plan'}:
        raise ValueError('Expected operation and plan only')
    operation = request['operation']
    if operation not in ('status', 'trace', 'capture', 'batch', 'setup'):
        raise ValueError('Operation outside approved primitives')
    if operation in ('status', 'trace', 'capture'):
        if request['plan'] is not None:
            raise ValueError('Observation takes no plan')
        return
    if operation == 'setup':
        if request['plan'] not in SETUP_ACTIONS:
            raise ValueError('Unknown fixed setup action')
    else:
        validate_plan(request['plan'])
    if set(trust['pins']) != {'source_commit', 'dll_sha256', 'stock_sha256', 'game_sha256'}:
        raise ValueError('Missing independently reviewed target pins')
    for key, size in (('source_commit', 40), ('dll_sha256', 64), ('stock_sha256', 64), ('game_sha256', 64)):
        if not re.fullmatch('[0-9a-f]{%d}' % size, trust['pins'][key]):
            raise ValueError('Invalid target pin')
        if operation == 'batch' and key != 'game_sha256' and request['plan'][key] != trust['pins'][key]:
            raise ValueError('Plan does not match approved target')


REMOTE = '''
request = json.load(sys.stdin)
operation = request['operation']
if operation == 'status':
    result = snapshot({})
    if result.get('safe_paths') and result.get('game'):
        result['game_sha256'] = sha(Path(result['game']) / 'Skate.exe')
elif operation == 'trace':
    result = native_trace({})
elif operation == 'capture':
    result = capture()
else:
    status = snapshot({})
    if not status.get('safe_paths') or sha(Path(status['game']) / 'Skate.exe') != request['game_sha256']:
        raise HardStop('game_integrity')
    for key, actual in (('source_commit', 'installed_commit'), ('dll_sha256', 'dll_sha256'), ('stock_sha256', 'backup_sha256')):
        if status.get(actual) != request['pins'][key]:
            raise HardStop('target_integrity')
    if operation == 'setup':
        actions = {'mount-board':(1,0x134,1), 'pause':(1,0x13b,1), 'back':(1,0x131,1),
                   'confirm':(1,0x130,1), 'up':(3,17,-1), 'down':(3,17,1),
                   'left':(3,16,-1), 'right':(3,16,1), 'tab-left':(1,0x136,1), 'tab-right':(1,0x137,1),
                   'subtab-left':(3,2,255), 'subtab-right':(3,5,255)}
        event = actions[request['plan']]
        result = experiment_events(request['plan'], [(0,[event]),(0.1,[(event[0],event[1],0)])], 1, capture_frames=1)
        print(json.dumps(result))
        sys.exit(0)
    plan = validate_plan(request['plan'])
    # One neutral device and its observers survive the whole validated batch.
    # Existing identity checks and per-trial integrity/reset checks stay in force.
    with ExperimentSession(max_seconds=1100) as session:
        result = perform_batch(plan, DeckEnvironment(session=session))
print(json.dumps(result))
'''


def execute(request, trust, sources):
    # Payload code comes exclusively from reviewed external files.
    payload = '\n'.join(sources[name] for name in ('worker_robot.py', 'worker_probe.py', 'plan_schema.py', 'worker_batch.py')) + REMOTE
    forwarded = dict(request, game_sha256=trust['pins'].get('game_sha256'), pins=trust['pins'])
    lab = ROOT.parent
    for name in ('known_hosts', 'id_ed25519'):
        path = lab / name
        if path.is_symlink() or not path.is_file():
            raise ValueError('Missing paired identity or host key')
    command = ['/usr/bin/ssh', '-F', '/dev/null', '-o', 'BatchMode=yes',
               '-o', 'StrictHostKeyChecking=yes', '-o', 'IdentitiesOnly=yes',
               '-o', 'UpdateHostKeys=no', '-o', 'PermitLocalCommand=no',
               '-o', 'ConnectTimeout=10', '-o', 'ConnectionAttempts=1',
               '-o', 'GlobalKnownHostsFile=/dev/null',
               '-o', 'UserKnownHostsFile=' + str(lab / 'known_hosts'),
               '-i', str(lab / 'id_ed25519'), trust['host'],
               '/usr/bin/python3 -I -B -c ' + shlex.quote(payload)]
    # Trusted output stays outside the mutable checkout and is never executable.
    folder = ROOT / 'results'
    if folder.is_symlink():
        raise ValueError('Unsafe output directory')
    folder.mkdir(mode=0o700, exist_ok=True)
    name = str(time.time_ns())
    path = folder / (name + '.json')
    with path.open('xb') as output:
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=output, stderr=subprocess.PIPE,
                                   cwd=ROOT, env={'PATH': '/usr/bin:/bin', 'HOME': str(Path.home())})
        try:
            _, error = process.communicate(json.dumps(forwarded).encode(), timeout=1200 if request['operation'] == 'batch' else 60)
        except BaseException:
            process.kill()
            process.wait()
            raise
        if process.returncode:
            raise RuntimeError('Trusted worker failed; no automatic input replay')
    if path.stat().st_size > 512 * 1024 * 1024:
        raise ValueError('Response exceeds evidence bound')
    print(json.dumps({'result': str(path), 'operation': request['operation']}))


def main():
    if len(sys.argv) != 1 or not sys.flags.isolated:
        raise ValueError('Use fixed isolated invocation; JSON arrives on stdin')
    trust, sources = load_bundle()
    data = sys.stdin.buffer.read(MAX_REQUEST + 1)
    if len(data) > MAX_REQUEST:
        raise ValueError('Request too large')
    request = decode(data)
    namespace = {}
    exec(compile(sources['plan_schema.py'], '<trusted schema>', 'exec'), namespace)
    validate_request(request, trust, namespace['validate_plan'])
    execute(request, trust, sources)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, RuntimeError) as exc:
        print(str(exc), file=sys.stderr)
        sys.exit(1)
