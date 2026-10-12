"""Offline Mac/Linux integration tests. Never connect to a real host or HOME."""
import ast
import base64
import contextlib
import fcntl
import hashlib
import io
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lab import controller, probe
import deck_installer as installer
import deck_updater as updater
import package_deck_installer as package
from test_deck_installer import fake_dll, game_at

STOPPED = {"skate": False, "launcher": False, "check_complete": True}


class Fixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.home = self.root / "deck-home"
        self.game = game_at(self.home / ".local/share/Steam/steamapps/common/skate")
        (self.game / installer.SETTINGS).write_text('{"updates":false,"width":1280,"other":"untouched"}')
        self.dll = fake_dll()
        self.commit = "a" * 40
        self.manifest = {"commit": self.commit, "sha256": installer.digest(self.dll)}
        self.dll_path = self.root / "build.dll"
        self.dll_path.write_bytes(self.dll)
        self.desktop = self.root / "Install.desktop"
        package.build(self.dll_path, self.commit, self.desktop)
        self.channel = json.loads(self.desktop.with_name("channel.json").read_text())
        self.fetch = Mock(side_effect=lambda url, *a: json.dumps(self.channel).encode()
                          if url == updater.CHANNEL else self.desktop.read_bytes())
        self.process_info = dict(STOPPED)
        self.commands = []
        self.lab = controller.Lab(directory=self.root / "mac-lab", game=str(self.game),
                                  run=self.fake_ssh, fetch=self.fetch)

    def fake_ssh(self, argv, **kwargs):
        self.commands.append(argv)
        self.assertEqual(argv[0], "ssh")
        code = shlex.split(argv[-1])
        self.assertEqual(code[:3], ["python3", "-B", "-c"])
        output = io.StringIO()
        # Run the actual serialized remote code with only external process state,
        # HOME and stdin substituted. The real installer writes the fake game.
        source = code[3].replace("def processes(", "def original_processes(")
        namespace = {"__name__": "fake_remote", "processes": lambda: self.process_info}
        with patch("pathlib.Path.home", return_value=self.home), patch("sys.stdin", io.StringIO(kwargs["input"])), \
             patch("subprocess.run", return_value=subprocess.CompletedProcess([], 1)), contextlib.redirect_stdout(output):
            exec(compile(source, "remote.py", "exec"), namespace)
        return subprocess.CompletedProcess(argv, 0, output.getvalue(), "")

    def snap(self):
        return probe.snapshot({"game": str(self.game), "dll_size": len(self.dll)},
                              home=self.home, media=self.root / "media", process_info=self.process_info)

    def contents(self):
        return {str(p.relative_to(self.home)): p.read_bytes() for p in self.home.rglob("*") if p.is_file()}

    def installed(self):
        with patch.object(installer, "ensure_stopped"):
            installer.change_installation(self.game, self.dll, self.commit)

    def log(self, text=None):
        path = self.game / "logs/ReSkate.log"
        path.parent.mkdir(exist_ok=True)
        content = text if text is not None else ('Arcade Mode ready (development observer).\n'
            '[2026-10-11 22:00:00.123] [42:7] SDK: Arcade mode initialized: active=true\n' +
            ''.join('ARCADE MODE DEV: ' + b + ' press #1 seen on ReSkate client tick.\n' for b in "ABXY"))
        path.write_text(content)
        receipt = json.loads((self.game / installer.RECEIPT).read_text()) if (self.game / installer.RECEIPT).exists() else {}
        timestamp = receipt.get("installed_at", 0) + 2
        os.utime(path, (timestamp, timestamp))
        return path


class LabTests(Fixture):
    def test_readonly_generated_code_ast(self):
        for operation in ("preflight", "status", "collect", "trace"):
            tree = ast.parse(controller.remote_code(operation))
            banned = {"write_text", "write_bytes", "mkdir", "unlink", "rename", "replace", "chmod", "touch",
                      "atomic_write", "change_installation", "exec", "eval", "system", "run", "Popen", "kill"}
            for node in ast.walk(tree):
                if isinstance(node, ast.Call):
                    name = getattr(node.func, "attr", getattr(node.func, "id", ""))
                    # str.replace only normalizes paths; it is not os.replace.
                    if name == "replace":
                        self.assertIsInstance(node.func.value, (ast.Name, ast.Subscript))
                        continue
                    self.assertNotIn(name, banned)
                    if name == "open":
                        self.assertEqual(ast.literal_eval(node.args[0]), "rb")
        self.assertNotIn("deck_installer", controller.remote_code("preflight"))

    def test_preflight_actual_remote_no_mutation(self):
        before = self.contents()
        result = self.lab.preflight()
        self.assertTrue(result["pass"])
        self.assertEqual(result["latest_tested_commit"], self.commit)
        self.assertEqual(result["expected_dll_sha256"], self.manifest["sha256"])
        self.assertEqual(before, self.contents())
        self.assertFalse(self.lab.directory.exists())

    def test_fake_deck_deploy_repeat_exact_restore(self):
        original = (self.game / "ReSkate.dll").read_bytes()
        settings = installer.load_settings(self.game)
        for _ in range(2):
            self.lab.deploy()
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        receipt = json.loads((self.game / installer.RECEIPT).read_text())
        self.assertEqual(receipt["commit"], self.commit)
        self.assertEqual(receipt["dll_sha256"], self.manifest["sha256"])
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), original)
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), original)
        self.assertEqual(installer.load_settings(self.game), settings)
        self.lab.remote("restore")
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), original)

    def test_deploy_running_refused(self):
        for name in ("skate", "launcher"):
            self.process_info[name] = True
            with self.assertRaises(RuntimeError):
                self.lab.deploy()
            self.process_info[name] = False
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_deploy_unknown_processes_refused(self):
        self.process_info["check_complete"] = False
        with self.assertRaises(RuntimeError):
            self.lab.deploy()

    def test_deploy_preflight_missing_file_refused(self):
        (self.game / "Skate.exe").unlink()
        with self.assertRaises(RuntimeError):
            self.lab.deploy()
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_remote_rechecks_running_before_deploy(self):
        self.process_info["launcher"] = True
        with self.assertRaises(RuntimeError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit=self.commit,
                            dll=base64.b64encode(self.dll).decode(), dll_size=len(self.dll))

    def test_remote_wrong_source_commit(self):
        with self.assertRaises(ValueError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit="b" * 40,
                            dll=base64.b64encode(self.dll).decode(), dll_size=len(self.dll))
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_remote_wrong_source_hash(self):
        with self.assertRaises(ValueError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit=self.commit,
                            dll=base64.b64encode(self.dll + b"corrupt").decode(), dll_size=len(self.dll))

    def test_destination_mismatch_rejected(self):
        with patch.object(self.lab, "remote", side_effect=[dict(self.snap(), **{"pass": True}),
                                                         {"dll_sha256": "0" * 64, "installed_commit": self.commit}]):
            with self.assertRaises(RuntimeError):
                self.lab.deploy()

    def test_ssh_host_key_policy(self):
        for pair in (False, True):
            args = controller.ssh_arguments("deck@steamdeck.local", self.root, pair)
            self.assertIn("StrictHostKeyChecking=" + ("ask" if pair else "yes"), args)
            self.assertIn("IdentitiesOnly=yes", args)
            self.assertNotIn("StrictHostKeyChecking=no", args)
            self.assertNotIn("accept-new", " ".join(args))
            self.assertNotIn("sshpass", args)

    def test_host_command_injection_rejected(self):
        for host in ("-oProxyCommand=bad", "deck@foo;touch x", "deck@$(id)"):
            with self.assertRaises(ValueError):
                controller.ssh_arguments(host, self.root)

    def test_pair_only_modifies_ssh_authorization(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake private key")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        run = Mock()
        self.lab.run = run
        before = self.contents()
        self.lab.pair()
        self.assertEqual(run.call_count, 1)
        args, options = run.call_args
        command = args[0][-1]
        self.assertIn("authorized_keys", command)
        for text in ("Skate", "installer", "config.json", "private key", "password"):
            self.assertNotIn(text, command)
        self.assertNotIn("input", options)
        self.assertNotIn("capture_output", options)
        self.assertEqual(before, self.contents())
        self.fetch.assert_not_called()

    def test_key_generation_ed25519_no_password_argument(self):
        self.lab.run = Mock()
        self.lab.key()
        args = self.lab.run.call_args.args[0]
        self.assertEqual(args[:3], ["ssh-keygen", "-t", "ed25519"])
        self.assertEqual(args[args.index("-N") + 1], "")

    def test_pair_remote_is_idempotent(self):
        # Execute only generated shell against a temporary HOME; fake key, no SSH.
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        self.lab.run = Mock()
        self.lab.pair()
        command = self.lab.run.call_args.args[0][-1]
        for _ in range(2):
            subprocess.run(["/bin/sh", "-c", command], env=dict(os.environ, HOME=str(self.home)), check=True)
        self.assertEqual((self.home / ".ssh/authorized_keys").read_text(), "ssh-ed25519 AAAA lab\n")

    def test_pair_changed_host_key_failure_never_deploys(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        self.lab.run = Mock(side_effect=subprocess.CalledProcessError(255, ["ssh"]))
        with self.assertRaises(subprocess.CalledProcessError):
            self.lab.pair()
        self.fetch.assert_not_called()
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_key_symlink_refused_before_keygen(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").symlink_to(self.root / "victim")
        self.lab.run = Mock()
        with self.assertRaises(RuntimeError):
            self.lab.key()
        self.lab.run.assert_not_called()

    def test_bootstrap_failure_writes_nothing(self):
        with patch.object(controller, "HOME", self.root / "bootstrap-home"), \
             patch.object(controller, "self_test", return_value=False), patch.object(controller.Lab, "key") as key:
            with self.assertRaises(RuntimeError):
                controller.bootstrap()
            self.assertFalse(controller.HOME.exists())
            key.assert_not_called()

    def test_bootstrap_success_never_deploys(self):
        destination = self.root / "bootstrap-home"
        with patch.object(controller, "HOME", destination), patch.object(controller, "self_test", return_value=True), \
             patch.object(controller, "Lab") as lab, patch("builtins.input", return_value="n"):
            lab.return_value.preflight.return_value = {"pass": True}
            controller.bootstrap()
            lab.return_value.deploy.assert_not_called()
            lab.return_value.pair.assert_not_called()
            lab.return_value.key.assert_called_once()
            self.assertTrue((destination / "controller/lab/controller.py").exists())

    def test_collect_sanitized_and_unknown_hardware_criteria(self):
        self.installed()
        self.log()
        self.process_info["skate"] = True
        path, published, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])
        self.assertTrue(all(report["buttons"].values()))
        self.assertIsNone(report["unexpected_crash"])
        self.assertEqual(report["observer_acceptance"], "UNVERIFIED")
        self.assertNotIn(str(self.home), path.read_text())
        self.assertNotIn("steamdeck.local", path.read_text())
        self.assertFalse(published)

    def test_collect_crash_detected(self):
        self.installed()
        path = self.log()
        path.write_text(path.read_text() + "ReSkate unhandled exception\nsecret IP 192.168.1.9\n")
        _, _, report = self.lab.collect(no_crash=True, publish=False)
        self.assertTrue(report["unexpected_crash"])
        self.assertEqual(report["observer_acceptance"], "FAIL")
        self.assertNotIn("192.168", json.dumps(report))

    def test_formatted_initialization_false_not_accepted(self):
        self.installed()
        self.log('Arcade Mode ready\nArcade mode initialized: active=false\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["arcade_mode_initialized"])

    def test_trace_json_initialization_supported(self):
        self.installed()
        self.log('Arcade Mode ready\nNative event: {"event":"arcade_mode_initialized","active":true}\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])

    def test_explicit_log_fallback_with_deploy_baseline(self):
        path = self.root / "proton/LocalAppData/ReSkate/logs/ReSkate.log"
        path.parent.mkdir(parents=True)
        self.lab.log = str(path)
        self.lab.deploy()
        path.write_text('Arcade Mode ready\n{"event":"arcade_mode_initialized","active":true}\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])

    def test_mismatched_dll_cannot_claim_runtime_acceptance(self):
        self.installed()
        self.log()
        (self.game / "ReSkate.dll").write_bytes(b"different")
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])

    def test_stale_log_not_accepted(self):
        self.log()
        self.installed()
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])
        self.assertFalse(report["arcade_mode_initialized"])

    def test_appended_old_log_not_accepted(self):
        path = self.log()
        self.installed()
        path.write_text(path.read_text() + "unrelated later output\n")
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])

    def test_latest_session_not_union_of_buttons(self):
        self.installed()
        path = self.log()
        path.write_text(path.read_text() + "Arcade Mode ready\nARCADE MODE DEV: A press #1 seen on ReSkate client tick.\n")
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["buttons"]["A"])
        self.assertFalse(report["buttons"]["B"])

    def test_restore_collect_retains_same_install_evidence(self):
        self.installed()
        self.log()
        self.process_info["skate"] = True
        self.lab.collect(holds=True, native=True, no_crash=True, publish=False)
        self.process_info["skate"] = False
        self.lab.remote("restore")
        _, _, report = self.lab.collect(publish=False)
        self.assertEqual(report["observer_acceptance"], "PASS")  # SIMULATED ONLY

    def test_new_install_cannot_reuse_old_acceptance(self):
        self.installed()
        self.log()
        self.lab.collect(holds=True, native=True, no_crash=True, publish=False)
        self.installed()
        self.lab.remote("restore")
        _, _, report = self.lab.collect(publish=False)
        self.assertEqual(report["observer_acceptance"], "UNVERIFIED")

    def test_publication_failure_keeps_local_report_and_dll(self):
        self.installed()
        self.log()
        with patch.object(controller, "publish_report", side_effect=FileNotFoundError("gh")):
            path, published, _ = self.lab.collect()
        self.assertTrue(path.is_file())
        self.assertFalse(published)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)

    def test_publication_pinned_orphan_branch_no_force(self):
        self.installed()
        self.log()
        _, _, report = self.lab.collect(publish=False)
        report["evil_secret"] = "do not publish"
        runner = Mock(side_effect=[subprocess.CompletedProcess([], 0, json.dumps(v)) for v in
                                  ([], {"sha": "b" * 40}, {"sha": "c" * 40}, {})])
        controller.publish_report(report, runner)
        calls = runner.call_args_list
        for call in calls:
            self.assertTrue(call.args[0][2].startswith("repos/notPho-Real/ReSkate/"))
            self.assertNotIn("evil_secret", str(call))
            self.assertNotIn("refs/heads/arcade-mode", str(call))
        self.assertEqual(json.loads(calls[2].kwargs["input"])["parents"], [])
        self.assertEqual(json.loads(calls[3].kwargs["input"])["ref"], "refs/heads/arcade-lab-results")

    def test_publication_existing_branch_fast_forward(self):
        self.installed()
        self.log()
        _, _, report = self.lab.collect(publish=False)
        values = ([{"ref": "refs/heads/arcade-lab-results", "object": {"sha": "d" * 40}}],
                  {"tree": {"sha": "e" * 40}}, {"sha": "f" * 40}, {"sha": "b" * 40}, {})
        runner = Mock(side_effect=[subprocess.CompletedProcess([], 0, json.dumps(v)) for v in values])
        controller.publish_report(report, runner)
        self.assertFalse(json.loads(runner.call_args.kwargs["input"])["force"])
        self.assertIn("PATCH", runner.call_args.args[0])


class TransactionTests(Fixture):
    def test_legacy_shell_helpers_use_shared_install_restore(self):
        source = self.dll_path.with_name("ReSkate.dll")
        source.write_bytes(self.dll)
        source.with_name("commit.txt").write_text(self.commit)
        source.with_name("ReSkate.dll.sha256").write_text(self.manifest["sha256"] + "  ReSkate.dll\n")
        binaries = self.root / "bin"
        binaries.mkdir()
        (binaries / "python3").symlink_to(sys.executable)
        pgrep = binaries / "pgrep"
        pgrep.write_text("#!/bin/sh\nexit 1\n")
        pgrep.chmod(0o755)
        env = dict(os.environ, PATH=str(binaries) + os.pathsep + os.environ.get("PATH", ""))
        subprocess.run(["bash", str(HERE / "update_arcade_dev.sh"), str(self.game), str(source)],
                       env=env, check=True, capture_output=True)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        subprocess.run(["bash", str(HERE / "restore_stock.sh"), str(self.game)],
                       env=env, check=True, capture_output=True)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_report_schema_rejects_bad_identity_and_discards_unknowns(self):
        self.installed()
        _, _, report = self.lab.collect(publish=False)
        report.update(unrelated_log="secret", unexpected_crash="secret")
        clean = controller.sanitize(report)
        self.assertNotIn("unrelated_log", clean)
        self.assertIsNone(clean["unexpected_crash"])
        self.assertEqual(clean["criteria"]["no_unexpected_crash"], "UNVERIFIED")
        report["source_commit"] = "../../bad"
        with self.assertRaises(ValueError):
            controller.sanitize(report)

    def test_invalid_manifest_values(self):
        for value in (None, {}, {"commit": [], "sha256": "a" * 64},
                      {"commit": "../bad", "sha256": "a" * 64}, {"commit": "a" * 40, "sha256": "bad"}):
            with self.assertRaises(ValueError):
                installer.validate_manifest(value)

    def test_pe_rejects_non_x64_non_dll_non_pe32plus(self):
        for offset, value in ((68, 0x14c), (86, 0), (88, 0x10b)):
            dll = bytearray(self.dll)
            struct.pack_into("<H", dll, offset, value)
            with self.assertRaises(RuntimeError):
                installer.validate_dll(bytes(dll))

    def test_ancestor_symlink_refused(self):
        link = self.root / "linked"
        link.symlink_to(self.game.parent, target_is_directory=True)
        with self.assertRaises(RuntimeError):
            installer.change_installation(link / self.game.name, self.dll, self.commit)

    def test_backup_receipt_journal_symlinks_refused(self):
        for name in (installer.BACKUP, installer.RECEIPT, installer.JOURNAL, installer.LOCK):
            path = self.game / name
            path.symlink_to(self.root / "victim")
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)
            path.unlink()
        self.assertFalse((self.root / "victim").exists())

    def test_flock_concurrency_refused(self):
        with (self.game / installer.LOCK).open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)

    def test_destination_corruption_rolls_back_all(self):
        before = installer.load_settings(self.game)
        real = installer.atomic_write
        once = [False]
        def corrupt(path, data):
            if path.name == "ReSkate.dll" and not once[0]:
                once[0] = True
                return real(path, b"corrupt")
            return real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=corrupt):
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertEqual(installer.load_settings(self.game), before)
        self.assertFalse((self.game / installer.RECEIPT).exists())

    def test_interrupted_settings_write_rolls_back(self):
        original = (self.game / installer.SETTINGS).read_bytes()
        real = installer.atomic_write
        once = [False]
        def interrupt(path, data):
            if path.name == installer.SETTINGS and not once[0]:
                once[0] = True
                real(path, data)
                raise KeyboardInterrupt()
            real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=interrupt):
            with self.assertRaises(KeyboardInterrupt):
                installer.change_installation(self.game, self.dll, self.commit)
        self.assertEqual((self.game / installer.SETTINGS).read_bytes(), original)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_killed_transaction_recovers_on_explicit_restore(self):
        self.installed()
        before = {name: base64.b64encode((self.game / name).read_bytes()).decode()
                  for name in ("ReSkate.dll", installer.SETTINGS, installer.RECEIPT)}
        (self.game / installer.JOURNAL).write_text(json.dumps(before))
        (self.game / "ReSkate.dll").write_bytes(b"partial transaction")
        self.assertFalse(self.snap()["pass"])
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertFalse((self.game / installer.JOURNAL).exists())

    def test_actual_process_exit_during_transaction_recovers(self):
        code = '''import sys, os
sys.path.insert(0, sys.argv[1])
import deck_installer as i
from pathlib import Path
i.ensure_stopped = lambda: None
real = i.atomic_write
def die(path, data):
    real(path, data)
    if path.name == 'ReSkate.dll': os._exit(77)
i.atomic_write = die
i.change_installation(Path(sys.argv[2]), Path(sys.argv[3]).read_bytes(), 'a'*40)
'''
        result = subprocess.run([sys.executable, "-B", "-c", code, str(HERE), str(self.game), str(self.dll_path)])
        self.assertEqual(result.returncode, 77)
        self.assertTrue((self.game / installer.JOURNAL).exists())
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertFalse(installer.load_settings(self.game)["updates"])

    def test_restore_write_failure_rolls_back_development(self):
        self.installed()
        real = installer.atomic_write
        def fail(path, data):
            if path.name == installer.SETTINGS:
                raise OSError("disk full")
            real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=fail):
            with self.assertRaises(OSError):
                installer.change_installation(self.game)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        self.assertTrue(json.loads((self.game / installer.RECEIPT).read_text())["active"])

    def test_changed_backup_refuses_restore(self):
        self.installed()
        (self.game / installer.BACKUP).write_bytes(b"tampered")
        with patch.object(installer, "ensure_stopped"), self.assertRaises(RuntimeError):
            installer.change_installation(self.game)

    def test_changed_backup_after_restore_refuses_reinstall(self):
        self.installed()
        with patch.object(installer, "ensure_stopped"):
            installer.change_installation(self.game)
        (self.game / installer.BACKUP).write_bytes(b"tampered")
        with patch.object(installer, "ensure_stopped"), self.assertRaises(RuntimeError):
            installer.change_installation(self.game, self.dll, self.commit)

    def test_unreadable_settings_report_failed_preflight(self):
        (self.game / installer.SETTINGS).write_text("invalid JSON")
        result = self.snap()
        self.assertFalse(result["pass"])
        self.assertFalse(result["settings_readable"])

    def test_backup_exclusive_creation(self):
        path = self.game / installer.BACKUP
        path.write_bytes(b"first")
        with self.assertRaises(RuntimeError):
            installer.preserve_backup(path, b"second")
        self.assertEqual(path.read_bytes(), b"first")

    def test_release_wrong_origin_hash_commit(self):
        for channel in (dict(self.channel, url="https://github.com/other/ReSkate/fake"),
                        dict(self.channel, sha256="0" * 64), dict(self.channel, commit=None)):
            with self.assertRaises(ValueError):
                updater.verified_release(lambda url, *a: json.dumps(channel).encode() if url == updater.CHANNEL else self.desktop.read_bytes())

    def test_release_validated_before_remote_use(self):
        self.channel["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.lab.deploy()
        self.assertEqual(self.commands, [])

    def test_low_space_preflight_refused(self):
        with patch.object(probe.os, "statvfs", return_value=type("Space", (), {"f_bavail": 0, "f_frsize": 4096})()):
            self.assertFalse(self.snap()["pass"])

    def test_process_detection_from_fake_proc(self):
        proc = self.root / "proc"
        (proc / "123").mkdir(parents=True)
        (proc / "123/cmdline").write_bytes(b"wine\0Z:\\games\\Skate.exe\0")
        self.assertTrue(probe.processes(proc)["skate"])
        self.assertFalse(probe.processes(proc)["launcher"])


class NativeTraceIntegrationTests(Fixture):
    def test_actual_serialized_trace_and_dll_identity(self):
        self.installed()
        self.log("Arcade Mode ready\nARCADE NATIVE CAPTURE: id=1 begin=true.\n"
                 "ARCADE NATIVE SAMPLE: id=1 ms=0 valid=true state=100 crouch_mode=2 words="
                 "00000001,00000002,00000003,00000004,00000005,00000006,00000007,00000008.\n"
                 "ARCADE NATIVE CAPTURE: id=1 end=true samples=1.\nprivate unrelated text\n")
        report = self.lab.remote("trace")
        self.assertEqual(report["source_commit"], self.commit)
        self.assertEqual(report["captures"][0]["samples"][0]["words"], list(range(1, 9)))
        self.assertTrue(report["captures"][0]["complete"])
        self.assertNotIn("private", json.dumps(report))
        (self.game / "ReSkate.dll").write_bytes(b"wrong DLL")
        with self.assertRaises(ValueError):
            self.lab.remote("trace")


class NativeTraceTests(unittest.TestCase):
    def test_numeric_allowlist_and_invalid_owner(self):
        text = "ARCADE NATIVE CAPTURE: id=2 begin=true.\n"
        text += "unrelated secret must never appear\n"
        text += "ARCADE NATIVE SAMPLE: id=2 ms=100 valid=false state=103 crouch_mode=2 words=" + ",".join(["ffffffff"]*8) + ".\n"
        text += "ARCADE NATIVE CAPTURE: id=2 end=true samples=1."
        result = probe.parse_native_captures(text)
        self.assertTrue(result[0]["complete"])
        self.assertEqual(result[0]["samples"][0]["words"], [0]*8)
        self.assertEqual(result[0]["samples"][0]["state"], 0)
        self.assertNotIn("secret", json.dumps(result))

    def test_capture_and_sample_bounds(self):
        text = ""
        for capture in range(10):
            text += f"ARCADE NATIVE CAPTURE: id={capture} begin=true.\n"
            text += (f"ARCADE NATIVE SAMPLE: id={capture} ms=1 valid=true state=103 crouch_mode=2 words=" + ",".join(["00000001"]*8) + ".\n")*130
        result = probe.parse_native_captures(text)
        self.assertEqual(len(result), 8)
        self.assertEqual(len(result[-1]["samples"]), 128)
        self.assertFalse(result[-1]["complete"])

    def test_wrong_id_and_expired_sample_rejected(self):
        text = "ARCADE NATIVE CAPTURE: id=2 begin=true.\n"
        for identity, milliseconds in ((1, 10), (2, 5000)):
            text += f"ARCADE NATIVE SAMPLE: id={identity} ms={milliseconds} valid=true state=100 crouch_mode=0 words=" + ",".join(["00000000"]*8) + ".\n"
        self.assertEqual(probe.parse_native_captures(text)[0]["samples"], [])

    def test_trace_transport_is_read_only(self):
        code = controller.remote_code("trace")
        self.assertNotIn("change_installation", code)
        self.assertIn("native_trace(json.load(sys.stdin))", code)
        with patch.object(probe, "evidence", return_value={"fresh_log": False}):
            with self.assertRaises(ValueError):
                probe.native_trace({})


if __name__ == "__main__":
    unittest.main()
