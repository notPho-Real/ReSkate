"""Security boundary regressions; these tests never contact hardware."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
LAB = Path(__file__).parent / 'lab'


def module(name):
    spec = importlib.util.spec_from_file_location(name, LAB / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


class PlanSecurity(unittest.TestCase):
    def setUp(self):
        self.schema = module('plan_schema')
        self.runner = module('trusted_runner')
        self.plan = json.loads((Path(__file__).parent / 'research/balance-runtime-08ab7dc.json').read_text())
        self.trust = {'pins': {key: self.plan[key] for key in ('source_commit', 'dll_sha256', 'stock_sha256')}}
        self.trust['pins']['game_sha256'] = 'a' * 64

    def test_bounded_plan_and_independent_pins(self):
        self.runner.validate_request({'operation': 'batch', 'plan': self.plan}, self.trust, self.schema.validate_plan)
        for key in self.trust['pins']:
            bad = copy.deepcopy(self.trust)
            bad['pins'][key] = 'b' * (40 if key == 'source_commit' else 64)
            if key == 'game_sha256':
                bad['pins'][key] = ''
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.runner.validate_request({'operation': 'batch', 'plan': self.plan}, bad, self.schema.validate_plan)

    def test_no_commands_paths_hosts_or_deployment(self):
        for field in ('command', 'shell', 'ssh', 'host', 'path', 'repository', 'script'):
            plan = copy.deepcopy(self.plan)
            plan[field] = 'untrusted'
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.schema.validate_plan(plan)
        for operation in ('shell', 'ssh', 'deploy', 'restore', 'delete', 'replace-game', 'sudo'):
            with self.subTest(operation=operation), self.assertRaises(ValueError):
                self.runner.validate_request({'operation': operation, 'plan': None}, self.trust, self.schema.validate_plan)

    def test_nested_unknown_fields_and_limits(self):
        for field in ('command', 'script', 'path'):
            plan = copy.deepcopy(self.plan)
            plan['candidates'][0]['phases'][0][field] = 'untrusted'
            with self.assertRaises(ValueError):
                self.schema.validate_plan(plan)
        for value in (True, -32769, 32768, float('nan')):
            plan = copy.deepcopy(self.plan)
            plan['candidates'][0]['phases'][0]['lx'] = value
            with self.assertRaises(ValueError):
                self.schema.validate_plan(plan)
        plan = copy.deepcopy(self.plan)
        plan['candidates'][0]['repeat'] = 25
        with self.assertRaises(ValueError):
            self.schema.validate_plan(plan)

    def test_json_ambiguities_rejected(self):
        for raw in ('{"operation":"batch","operation":"status"}', '{"x":NaN}', '{"x":Infinity}'):
            with self.assertRaises(ValueError):
                self.runner.decode(raw)

    def test_observation_has_no_arguments(self):
        for operation in ('status', 'trace', 'capture'):
            self.runner.validate_request({'operation': operation, 'plan': None}, self.trust, self.schema.validate_plan)
            with self.assertRaises(ValueError):
                self.runner.validate_request({'operation': operation, 'plan': {'game': '/tmp'}}, self.trust, self.schema.validate_plan)

    def test_reviewed_source_transport_preserves_code_and_stdin(self):
        source = ('# reviewed source with quotes \'\" and Unicode: café\n' * 4000
                  + 'import json,sys\nprint(json.dumps(json.load(sys.stdin)))\n')
        command = shlex.split(self.runner.remote_command(source))
        self.assertLess(len(self.runner.remote_command(source).encode()), 65536)
        self.assertEqual(command[:4], ['/usr/bin/python3', '-I', '-B', '-c'])
        request = {'operation': 'status', 'plan': None, 'text': '__import__("os").abort()'}
        result = subprocess.run([sys.executable, *command[1:]], input=json.dumps(request),
                                text=True, capture_output=True, check=True, timeout=10)
        self.assertEqual(json.loads(result.stdout), request)
        with self.assertRaisesRegex(ValueError, 'source exceeds transport bound'):
            self.runner.remote_command('x' * (1024 * 1024 + 1))

    def test_incompressible_source_rejected_before_transport(self):
        import base64
        import random
        data = random.Random(42).randbytes(100000)
        with self.assertRaisesRegex(ValueError, 'source exceeds command bound'):
            self.runner.remote_command('# ' + base64.b64encode(data).decode('ascii'))

    def test_setup_has_only_fixed_reviewed_actions(self):
        for action in self.runner.SETUP_ACTIONS:
            self.runner.validate_request({'operation': 'setup', 'plan': action}, self.trust, self.schema.validate_plan)
        for action in ('shell', 'sudo', 'replace-game', 'delete-backup', ['pause'], {'key': 'Escape'}):
            with self.assertRaises(ValueError):
                self.runner.validate_request({'operation': 'setup', 'plan': action}, self.trust, self.schema.validate_plan)

    def test_external_bundle_rejects_tampering_and_foreign_scope(self):
        import hashlib
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            manifest = {'version': self.runner.VERSION, 'repository': 'notPho-Real/ReSkate',
                        'host': 'deck@steamdeck.local', 'pins': {}, 'files': {}}
            for name in self.runner.FILES:
                data = b'# reviewed fixture\n'
                (root / name).write_bytes(data)
                manifest['files'][name] = hashlib.sha256(data).hexdigest()
            (root / 'trust.json').write_text(json.dumps(manifest))
            with patch.object(self.runner, 'ROOT', root):
                self.runner.load_bundle()
                for field, value in (('host', 'other@example.invalid'), ('repository', 'other/project')):
                    bad = dict(manifest, **{field: value})
                    (root / 'trust.json').write_text(json.dumps(bad))
                    with self.assertRaises(ValueError):
                        self.runner.load_bundle()
                (root / 'trust.json').write_text(json.dumps(manifest))
                (root / 'worker_batch.py').write_text('# replaced checkout code\n')
                with self.assertRaises(ValueError):
                    self.runner.load_bundle()
                (root / 'worker_batch.py').unlink()
                (root / 'worker_batch.py').symlink_to(root / 'worker_probe.py')
                with self.assertRaises(ValueError):
                    self.runner.load_bundle()


class WorkflowSecurity(unittest.TestCase):
    def test_naming_guard_rejects_content_and_message(self):
        guard = module('naming_guard')
        for word in guard.WORDS:
            value = ''.join(chr(c) for c in word)
            self.assertTrue(guard.forbidden(value))
            self.assertTrue(guard.forbidden('message: ' + value.upper()))
        self.assertFalse(guard.forbidden('Arcade Mode Classic Controls Kickflip'))

    def test_permissions_and_action_pins(self):
        for path in (ROOT / '.github/workflows').glob('arcade-*.yml'):
            text = path.read_text()
            self.assertNotIn('pull_request_target', text)
            self.assertNotIn('write-all', text)
            header, jobs = text.split('jobs:\n', 1)
            self.assertIn('permissions:\n  contents: read\n', header)
            for action in re.findall(r'uses: (\S+)', text):
                self.assertRegex(action, r'^actions/(checkout|upload-artifact|download-artifact)@[0-9a-f]{40}$')
            for part in re.split(r'(?m)^  (?=[a-z][a-z-]*:\s*$)', jobs):
                if not part.strip():
                    continue
                name = part.split(':', 1)[0].strip()
                if 'contents: write' in part:
                    self.assertEqual(name, 'publish')
                    self.assertIn("github.ref == 'refs/heads/arcade-mode'", part.split('steps:', 1)[0])
                    self.assertIn("github.event_name == 'push'", part.split('steps:', 1)[0])
                    self.assertNotIn('actions/checkout@', part)
                    self.assertNotIn('python3 contrib/', part)
                    self.assertIn('cmp artifacts/channel.json', part)
                    self.assertIn('needs: [build, linux-tests, macos-lab, deck]', part)

    def test_repository_has_no_execution_allowlist(self):
        self.assertNotIn('decision = "allow"', (ROOT / '.codex/rules/arcade-lab.rules').read_text())


if __name__ == '__main__':
    unittest.main()
