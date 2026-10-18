import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / 'lab'))

import publication_guard as guard
import install_publication_hook as installer


class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.git('init', '-q', '-b', 'arcade-mode')
        self.git('config', 'user.name', 'Fixture')
        self.git('config', 'user.email', 'fixture@example.invalid')
        self.git('remote', 'add', 'origin', guard.TARGET)
        self.commit('base', b'neutral baseline\n')
        self.base = self.git('rev-parse', 'HEAD').strip().decode()

    def git(self, *args):
        return subprocess.run(['/usr/bin/git', '-C', str(self.root), *args],
                              check=True, capture_output=True).stdout

    def commit(self, message='neutral change', data=b'neutral text\n', path='data.txt'):
        (self.root / path).write_bytes(data)
        self.git('add', '--', path)
        self.git('commit', '-q', '-m', message)
        return self.git('rev-parse', 'HEAD').strip().decode()

    def update(self):
        return f'{guard.REF} {self.git("rev-parse", "HEAD").strip().decode()} {guard.REF} {self.base}\n'

    def inspect(self, update=None):
        return guard.inspect(self.root, 'origin', guard.TARGET, update or self.update())

    def bad(self):
        return bytes(guard.__dict__['forbidden'].__globals__['WORDS'][0])

    def test_clean_multiple_commits(self):
        self.commit()
        self.commit(data=b'next\n')
        self.assertEqual(len(self.inspect()['manifest']['commits']), 2)

    def test_earlier_bad_tree_cleaned_tip(self):
        self.commit(data=self.bad())
        self.commit(data=b'cleaned\n')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_earlier_bad_message_clean_tip(self):
        self.commit(message=self.bad().decode())
        self.commit(data=b'next\n')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_side_branch_bad_tree_cleaned_before_merge(self):
        self.git('checkout', '-q', '-b', 'fixture-side')
        self.commit(data=self.bad())
        self.commit(data=b'cleaned side\n')
        self.git('checkout', '-q', 'arcade-mode')
        self.commit(path='parallel.txt', data=b'neutral independent change\n')
        self.git('merge', '--no-ff', '-m', 'neutral merge', 'fixture-side')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_bad_filename(self):
        self.commit(path=self.bad().decode())
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_binary_generated(self):
        self.commit(data=b'\x00\xffneutral')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_binary_embedded_name(self):
        with self.assertRaises(guard.Rejected):
            guard.scan(b'\xff\0' + self.bad())

    def test_utf16_name(self):
        with self.assertRaises(guard.Rejected):
            guard.scan(self.bad().decode().encode('utf-16-le'))

    def test_private_path(self):
        with self.assertRaises(guard.Rejected):
            guard.scan(bytes((47, 85, 115, 101, 114, 115, 47)) + b'fixture-owner/project')

    def test_private_endpoint_and_secret(self):
        for value in [bytes((49, 57, 50, 46, 49, 54, 56, 46, 50, 46, 51)),
                      b'fixture' + b'.' + b'local', b'gh' + b'p_' + b'x' * 24]:
            with self.subTest(value_hash=__import__('hashlib').sha256(value).hexdigest()):
                with self.assertRaises(guard.Rejected):
                    guard.scan(value)

    def test_vendor_license_and_generic_sqlite_examples(self):
        guard.scan(b'Copyright Example Authors. Licensed under MIT.\n'
                   b'SQLite example: /path/to/database.sqlite\n'
                   b'Example address 192.0.2.10; /home/example/project\n')
        guard.scan(bytes((67, 58, 92, 85, 115, 101, 114, 115, 92)) + b'you\\Music')

    def test_uri_home_paths_and_prose_hosts(self):
        home = bytes((47, 85, 115, 101, 114, 115, 47)) + b'person/private'
        for value in [home, b'file://' + home, b'https://example.invalid' + home,
                      b'machine' + b'.local is the host', b'machine' + b'.internal, endpoint']:
            with self.assertRaises(guard.Rejected):
                guard.scan(value)

    def test_exact_coding_branch_promotion(self):
        self.git('checkout', '-q', '-b', 'arcade-coding')
        tip = self.commit()
        update = f'{guard.CODING_REF} {tip} {guard.REF} {self.base}\n'
        self.assertEqual(self.inspect(update)['manifest']['commits'], [tip])
        with self.assertRaises(guard.Rejected):
            self.inspect(update.replace(guard.CODING_REF, 'refs/heads/arbitrary'))
        self.git('checkout', '-q', 'arcade-mode')
        with self.assertRaises(guard.Rejected):
            self.inspect(update)

    def test_exact_reviewed_example_does_not_allow_modified_copy(self):
        data = bytes((47, 104, 111, 109, 101, 47)) + b'fixture-owner/project'
        with patch.object(guard, 'PUBLIC_EXAMPLES', {__import__('hashlib').sha256(data).hexdigest()}):
            guard.scan(data)
            with self.assertRaises(guard.Rejected):
                guard.scan(data + b' changed')
            with self.assertRaises(guard.Rejected):
                guard.scan(self.bad())

    def test_preserved_vendor_binary_scanned(self):
        self.commit(data=b'\x89\xff\0neutral', path='vendor.bin')
        self.base = self.git('rev-parse', 'HEAD').strip().decode()
        self.commit(data=b'new text\n')
        self.inspect()

    def test_index_only_violation(self):
        (self.root / 'new.txt').write_bytes(self.bad())
        self.git('add', 'new.txt')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_untracked_generated_violation(self):
        (self.root / 'generated.log').write_bytes(self.bad())
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_unstaged_violation(self):
        (self.root / 'data.txt').write_bytes(self.bad())
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_working_change_invalidates_receipt(self):
        before = self.inspect()['digest']
        (self.root / 'new.txt').write_text('neutral generated artifact')
        self.assertNotEqual(before, self.inspect()['digest'])

    def test_working_mode_invalidates_receipt(self):
        before = self.inspect()['digest']
        (self.root / 'data.txt').chmod(0o755)
        self.assertNotEqual(before, self.inspect()['digest'])

    def test_grafted_history_rejected(self):
        (self.root / '.git' / 'info' / 'grafts').write_text(self.base + '\n')
        with self.assertRaises(guard.Rejected):
            self.inspect()

    def test_unsupported_updates(self):
        original = self.update()
        values = ['', original + original, original.replace(guard.REF, 'refs/tags/example'),
                  original.replace(self.base, guard.ZERO),
                  f'{guard.REF} {guard.ZERO} {guard.REF} {self.base}\n', 'malformed',
                  original.replace(self.base, 'z' * 40)]
        for value in values:
            with self.subTest(case=value[:15]):
                with self.assertRaises(guard.Rejected):
                    guard.inspect(self.root, 'origin', guard.TARGET, value)

    def test_non_fast_forward(self):
        later = self.commit()
        self.git('reset', '--hard', self.base)
        with self.assertRaises(guard.Rejected):
            self.inspect(self.update().replace(self.base + '\n', later + '\n'))

    def test_wrong_target(self):
        with self.assertRaises(guard.Rejected):
            guard.inspect(self.root, 'other', guard.TARGET, self.update())

    def test_scanner_failure(self):
        with patch.object(guard, 'scan', side_effect=OSError('fixture failure')):
            with self.assertRaises(OSError):
                self.inspect()

    def test_missing_object(self):
        with self.assertRaises(guard.Rejected):
            self.inspect(self.update().replace(self.base + '\n', 'f' * 40 + '\n'))

    def test_cli_requires_exact_semantic_receipt(self):
        script = str(Path(guard.__file__).resolve())
        receipt = self.root.parent / (self.root.name + '-receipt.json')
        self.addCleanup(lambda: receipt.unlink(missing_ok=True))
        cmd = [__import__('sys').executable, '-I', '-B', script, 'origin', guard.TARGET,
               '--root', str(self.root), '--review-file', str(receipt)]
        def run():
            return subprocess.run(cmd, input=self.update().encode(), capture_output=True)
        self.assertNotEqual(run().returncode, 0)
        report = self.inspect()
        receipt.write_text(json.dumps({'digest': report['digest'], 'semantic_review_complete': True}))
        self.assertEqual(run().returncode, 0)
        receipt.write_text(json.dumps({'digest': 'stale', 'semantic_review_complete': True}))
        self.assertNotEqual(run().returncode, 0)

    def test_hook_preserves_and_chains_existing_hooks(self):
        old = self.root / '.git' / 'hooks'
        original = b'#!/bin/sh\nexit 0\n'
        (old / 'pre-commit').write_bytes(original)
        (old / 'pre-commit').chmod(0o700)
        (old / 'pre-push').write_bytes(original)
        (old / 'pre-push').chmod(0o700)
        destination = self.root.parent / (self.root.name + '-trusted-hooks')
        self.addCleanup(lambda: __import__('shutil').rmtree(destination, ignore_errors=True))
        installer.install(self.root, destination)
        self.assertEqual((old / 'pre-commit').read_bytes(), original)
        self.assertEqual((destination / 'pre-commit').read_bytes(), original)
        self.assertEqual((destination / 'prior-pre-push').read_bytes(), original)
        command = [str(destination / 'pre-push'), 'origin', guard.TARGET]
        def run():
            return subprocess.run(command, input=self.update().encode(), cwd=self.root,
                                  capture_output=True)
        self.assertNotEqual(run().returncode, 0)
        report = self.inspect()
        (destination / 'review.json').write_text(json.dumps(
            {'digest': report['digest'], 'semantic_review_complete': True}))
        # The trusted copy includes the hook's identity in its inventory.
        report_command = ['/usr/bin/python3', '-I', '-B', str(destination / 'publication_guard.py'),
                          'origin', guard.TARGET, '--root', str(self.root), '--inventory']
        report = json.loads(subprocess.run(report_command, input=self.update().encode(),
                                          check=True, capture_output=True).stdout)
        (destination / 'review.json').write_text(json.dumps(
            {'digest': report['digest'], 'semantic_review_complete': True}))
        self.assertEqual(run().returncode, 0)
        scanner_bytes = (destination / 'publication_guard.py').read_bytes()
        (destination / 'publication_guard.py').write_bytes(scanner_bytes + b'\n# neutral change\n')
        self.assertNotEqual(run().returncode, 0)
        (destination / 'publication_guard.py').write_bytes(scanner_bytes)
        # Prove the original pre-push is still effective.
        (destination / 'prior-pre-push').write_text('#!/bin/sh\nexit 17\n')
        self.assertEqual(run().returncode, 17)

    def test_real_git_push_is_blocked_without_publication(self):
        destination = self.root.parent / (self.root.name + '-push-hooks')
        remote = self.root.parent / (self.root.name + '-remote.git')
        self.addCleanup(lambda: __import__('shutil').rmtree(destination, ignore_errors=True))
        self.addCleanup(lambda: __import__('shutil').rmtree(remote, ignore_errors=True))
        subprocess.run(['/usr/bin/git', 'init', '-q', '--bare', str(remote)], check=True)
        self.git('remote', 'set-url', 'origin', str(remote))
        installer.install(self.root, destination)
        result = subprocess.run(['/usr/bin/git', '-C', str(self.root), 'push', 'origin',
                                 'arcade-mode'], capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b'Publication blocked', result.stderr)
        result = subprocess.run(['/usr/bin/git', '-C', str(remote), 'rev-parse',
                                 '--verify', 'refs/heads/arcade-mode'], capture_output=True)
        self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
