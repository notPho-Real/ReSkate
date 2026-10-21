"""Validate bounded session sidecar bytes; no file/host IO or admission change.

The caller must independently verify the receipt and active PID/creation time.
This observes the backing file of the loader-selected module, not loaded memory.
Existing evidence()/native_trace() retain their original fail-closed admission.
"""
import re

MAX_BYTES = 2 * 32 * 1024
HEADER = re.compile(rb'SESSION schema=1 pid=([1-9][0-9]{0,9}) process_created_100ns=([1-9][0-9]{0,19}) observed_100ns=([1-9][0-9]{0,19}) (.{1,4096})\n')
MODULE = re.compile(rb'role=runtime phase=module_observed module_file_sha256=([0-9a-f]{64}) loaded_memory_sha256=UNVERIFIED')


def validate_session(data, *, pid, process_created_100ns, installed_at_100ns, module_file_sha256):
    if not isinstance(data, bytes) or len(data) > MAX_BYTES or (data and not data.endswith(b'\n')):
        raise ValueError('Oversized or partial session evidence')
    if not re.fullmatch('[0-9a-f]{64}', module_file_sha256):
        raise ValueError('Invalid expected module hash')
    if any(type(v) is not int or v <= 0 for v in (pid, process_created_100ns, installed_at_100ns)):
        raise ValueError('Missing independently verified process/deployment generation')
    records = []
    latest_module = None
    for line in data.splitlines(keepends=True):
        match = HEADER.fullmatch(line)
        if not match:
            raise ValueError('Malformed session record')
        owner, created, observed = map(int, match.groups()[:3])
        if observed < created:
            raise ValueError('Session observation predates process')
        body = match[4]
        module = MODULE.fullmatch(body)
        if module:
            latest_module = (owner, created, observed, module[1].decode())
        if (owner, created) == (pid, process_created_100ns):
            records.append((observed, body))
    expected = (pid, process_created_100ns)
    if latest_module is None or latest_module[:2] != expected or latest_module[3] != module_file_sha256:
        raise ValueError('Newest runtime module record does not match active process and installed hash')
    if process_created_100ns <= installed_at_100ns or latest_module[2] <= installed_at_100ns:
        raise ValueError('Stale process or module observation')
    if any(a[0] > b[0] for a, b in zip(records, records[1:])):
        raise ValueError('Reordered session observations')
    initialized = any(body == b'role=runtime phase=initialized image_validation=passed' for _, body in records)
    ready = any(body == b'role=runtime phase=arcade_ready native_action_acceptance=UNVERIFIED' for _, body in records)
    return {'pid': pid, 'process_created_100ns': process_created_100ns,
            'module_file_sha256': module_file_sha256, 'module_file_match': True,
            'runtime_initialized': initialized, 'arcade_ready_recorded': ready,
            'original_observed_100ns': [at for at, _ in records],
            'loaded_memory_identity': 'UNVERIFIED', 'fresh_log': False,
            'capture_complete': False, 'native_action_acceptance': 'UNVERIFIED'}
