import unittest
from lab.session_records import validate_session, MAX_BYTES

HASH = 'b'*64


def record(body, pid=7, created=200, at=300):
    return f'SESSION schema=1 pid={pid} process_created_100ns={created} observed_100ns={at} {body}\n'.encode()


def module(**kwargs):
    return record(f'role=runtime phase=module_observed module_file_sha256={HASH} loaded_memory_sha256=UNVERIFIED', **kwargs)


def validate(data, **kwargs):
    return validate_session(data, pid=7, process_created_100ns=200, installed_at_100ns=100,
                            module_file_sha256=kwargs.get('hash', HASH))


class SessionRecordsTests(unittest.TestCase):
    def test_original_generation_timestamps_and_no_acceptance_upgrade(self):
        data = module()+record('role=runtime phase=arcade_ready native_action_acceptance=UNVERIFIED', at=301)
        data += record('role=runtime phase=initialized image_validation=passed', at=302)
        result = validate(data)
        self.assertTrue(result['module_file_match'])
        self.assertTrue(result['arcade_ready_recorded'])
        self.assertTrue(result['runtime_initialized'])
        self.assertEqual(result['original_observed_100ns'], [300,301,302])
        self.assertFalse(result['fresh_log'])
        self.assertFalse(result['capture_complete'])
        self.assertEqual(result['loaded_memory_identity'], 'UNVERIFIED')

    def test_prior_session_prefix_does_not_supply_new_ready(self):
        old = module(pid=6,created=150)+record('role=runtime phase=arcade_ready native_action_acceptance=UNVERIFIED',pid=6,created=150)
        self.assertFalse(validate(old+module())['arcade_ready_recorded'])

    def test_newer_different_runtime_rejects_old_matching_prefix(self):
        with self.assertRaises(ValueError): validate(module()+module(pid=8,created=250))

    def test_pid_reuse_is_not_session_identity(self):
        with self.assertRaises(ValueError): validate(module(created=150))

    def test_partial_record_rejected(self):
        with self.assertRaises(ValueError): validate(module()[:-1])

    def test_size_budget(self):
        with self.assertRaises(ValueError): validate(b'x'*(MAX_BYTES+1))

    def test_hash_mismatch(self):
        with self.assertRaises(ValueError): validate(module(),hash='c'*64)

    def test_missing_module_rejected(self):
        with self.assertRaises(ValueError): validate(record('role=runtime phase=initialized image_validation=passed'))

    def test_reordered_or_precreation_timestamp_rejected(self):
        for data in [module(at=199), module()+record('role=runtime phase=initialized image_validation=passed',at=250)]:
            with self.assertRaises(ValueError): validate(data)


if __name__ == '__main__': unittest.main()
