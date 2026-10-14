"""Explicit Mac lab operations. All transport is injectable for offline tests."""
import argparse
import base64
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import deck_installer as installer
import deck_updater as updater

REPO = "notPho-Real/ReSkate"
RESULT_BRANCH = "arcade-lab-results"
HOME = Path.home() / ".local/share/reskate-arcade-lab"


def ssh_arguments(host, directory, pair=False):
    if not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*@[A-Za-z0-9][A-Za-z0-9_.:-]*", host):
        raise ValueError("Expected user@host")
    return ["ssh", "-F", "/dev/null", "-o", "ConnectTimeout=10", "-o", "ConnectionAttempts=1",
            "-o", "StrictHostKeyChecking=" + ("ask" if pair else "yes"),
            "-o", "UserKnownHostsFile=" + str(directory / "known_hosts"),
            "-o", "GlobalKnownHostsFile=/dev/null", "-o", "IdentitiesOnly=yes",
            "-o", "UpdateHostKeys=no",
            "-o", "BatchMode=" + ("no" if pair else "yes"),
            "-i", str(directory / "id_ed25519"), host]


def remote_code(operation):
    probe = (HERE / "probe.py").read_text()
    if operation in ("preflight", "status", "collect", "trace"):
        function = "native_trace" if operation == "trace" else "evidence" if operation == "collect" else "snapshot"
        return probe + '\nimport sys\nprint(json.dumps(' + function + '(json.load(sys.stdin))))\n'
    if operation not in ("deploy", "restore"):
        raise ValueError("Unknown remote operation")
    # Trusted checked-out controller code, not code supplied by an HTTP response.
    source = base64.b64encode((HERE.parent / "deck_installer.py").read_bytes()).decode()
    return probe + '\n' + """
import sys, base64
request = json.load(sys.stdin)
namespace = {'__name__': 'arcade_lab_installer'}
exec(compile(base64.b64decode(SOURCE), 'deck_installer.py', 'exec'), namespace)
status = snapshot(request)
if OPERATION == 'deploy':
    if not status['pass']:
        raise RuntimeError('Preflight failed; no install attempted')
    manifest = namespace['validate_manifest'](request['manifest'], request['expected_commit'])
    dll = base64.b64decode(request['dll'], validate=True)
    if namespace['digest'](dll) != manifest['sha256']:
        raise ValueError('Source DLL hash mismatch')
    namespace['change_installation'](status['game'], dll, manifest['commit'], log_path=request.get('log'))
else:
    if (not status.get('safe_paths') or not all(status.get('files', {}).values())
        or not status['processes']['check_complete'] or status['processes']['skate']
        or status['processes']['launcher']):
        raise RuntimeError('Restore requires safe paths and stopped processes')
    namespace['change_installation'](status['game'])
after = snapshot(request)
if OPERATION == 'deploy' and after['dll_sha256'] != manifest['sha256']:
    raise RuntimeError('Destination verification failed')
print(json.dumps(after))
""".replace("SOURCE", repr(source)).replace("OPERATION", repr(operation))


class Lab:
    def __init__(self, host="deck@steamdeck.local", directory=HOME, game=None,
                 run=subprocess.run, fetch=updater.download, log=None):
        self.host, self.directory, self.game = host, Path(directory), game
        self.run, self.fetch = run, fetch
        self.log = log
        ssh_arguments(host, self.directory)  # Validate before any external action.

    def key(self):
        installer.safe_path(self.directory)
        self.directory.mkdir(parents=True, exist_ok=True, mode=0o700)
        key = self.directory / "id_ed25519"
        for path in (key, key.with_suffix(".pub"), self.directory / "known_hosts"):
            installer.safe_path(path)
        if not key.exists():
            self.run(["ssh-keygen", "-t", "ed25519", "-f", str(key), "-N", "", "-C", "reskate-arcade-lab"], check=True)
        if key.is_symlink() or key.with_suffix(".pub").is_symlink():
            raise ValueError("Unsafe key path")

    def pair(self):
        self.key()
        public = (self.directory / "id_ed25519.pub").read_text().strip()
        if not re.fullmatch(r"ssh-ed25519 [A-Za-z0-9+/=]+(?: [^\r\n]*)?", public):
            raise ValueError("Invalid project public key")
        # stdin goes directly to ssh for its terminal password prompt; never capture it.
        command = ("umask 077; test ! -L ~/.ssh && mkdir -p ~/.ssh && "
                   "test ! -L ~/.ssh/authorized_keys && touch ~/.ssh/authorized_keys && "
                   "(grep -qxF -- " + shlex.quote(public) + " ~/.ssh/authorized_keys || printf '%s\\n' "
                   + shlex.quote(public) + " >> ~/.ssh/authorized_keys)")
        self.run(ssh_arguments(self.host, self.directory, pair=True) + [command], check=True)
        return "PAIR COMPLETE — NO GAME FILES CHANGED"

    def remote(self, operation, **values):
        request = dict(game=self.game, **values)
        command = "python3 -B -c " + shlex.quote(remote_code(operation))
        result = self.run(ssh_arguments(self.host, self.directory) + [command],
                          input=json.dumps(request), capture_output=True, text=True, check=True, timeout=60)
        return json.loads(result.stdout)

    def preflight(self, release=None):
        manifest, dll = release if release is not None else updater.verified_release(self.fetch)
        result = self.remote("preflight", dll_size=len(dll))
        result.update(latest_tested_commit=manifest["commit"], expected_dll_sha256=manifest["sha256"])
        return result

    def deploy(self):
        release = updater.verified_release(self.fetch)
        manifest, dll = release
        status = self.preflight(release)
        if not status.get("pass") or status["processes"]["skate"] or status["processes"]["launcher"]:
            raise RuntimeError("Preflight failed; deployment refused")
        result = self.remote("deploy", manifest=manifest, expected_commit=manifest["commit"],
                             dll=base64.b64encode(dll).decode(), dll_size=len(dll), log=self.log)
        if result.get("dll_sha256") != manifest["sha256"] or result.get("installed_commit") != manifest["commit"]:
            raise RuntimeError("Destination verification failed; inspect status and restore if necessary")
        return result

    def collect(self, log=None, holds=None, native=None, no_crash=None, publish=True):
        raw = self.remote("collect", log=log)
        report = sanitize(raw)
        self.directory.mkdir(parents=True, exist_ok=True)
        folder = self.directory / "results"
        folder.mkdir(exist_ok=True)
        path = folder / (report["source_commit"] + ".json")
        # A post-restore collection may retain prior runtime evidence only for this
        # exact installation, never for another deployment of the same source SHA.
        if path.exists() and report["stock_restore"] == "PASS":
            previous = sanitize(json.loads(path.read_text()))
            if (previous["installation_id"] == report["installation_id"]
                    and previous["installed_dll_sha256"] == report["installed_dll_sha256"]):
                for name in ("reskate_process_seen", "arcade_mode_initialized", "arcade_mode_ready",
                             "buttons", "fresh_log", "unexpected_crash", "holds", "native_controls"):
                    report[name] = previous[name]
        if holds is not None:
            report["holds"] = "PASS" if holds else "FAIL"
        if native is not None:
            report["native_controls"] = "PASS" if native else "FAIL"
        if no_crash is not None and report["unexpected_crash"] is not True:
            report["unexpected_crash"] = not no_crash
        report = sanitize(report)
        installer.atomic_write(path, installer.encode(report))
        published = False
        if publish:
            try:
                publish_report(report, self.run)
                published = True
            except (OSError, ValueError, subprocess.CalledProcessError):
                print("Publication blocked; complete sanitized report saved locally. Installation unchanged.")
        return path, published, report


def sanitize(value):
    installer.validate_manifest({"commit": value.get("source_commit"), "sha256": value.get("installed_dll_sha256")})
    identity = value.get("installation_id", "")
    timestamp = value.get("timestamp", "")
    if not isinstance(identity, str) or not re.fullmatch("[0-9a-f]{32}", identity):
        raise ValueError("Invalid installation identity")
    if not isinstance(timestamp, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z", timestamp):
        raise ValueError("Invalid report timestamp")
    report = {"schema": 1, "source_commit": value["source_commit"],
              "installed_dll_sha256": value["installed_dll_sha256"],
              "installation_id": identity, "timestamp": timestamp}
    for name in ("reskate_process_seen", "arcade_mode_initialized", "arcade_mode_ready", "fresh_log"):
        report[name] = value.get(name) is True
    report["buttons"] = {button: value.get("buttons", {}).get(button) is True for button in "ABXY"}
    crash = value.get("unexpected_crash")
    report["unexpected_crash"] = crash if type(crash) is bool else None
    for name in ("holds", "native_controls", "stock_restore", "updater_restore"):
        item = value.get(name)
        report[name] = item if item in ("PASS", "FAIL") else "UNVERIFIED"
    criteria = {name: "PASS" if report[name] else "UNVERIFIED" for name in
                ("reskate_process_seen", "arcade_mode_initialized", "arcade_mode_ready", "fresh_log")}
    criteria.update({"button_" + b: "PASS" if report["buttons"][b] else "UNVERIFIED" for b in "ABXY"})
    criteria.update({name: report[name] for name in ("holds", "native_controls", "stock_restore", "updater_restore")})
    crash = report["unexpected_crash"]
    criteria["no_unexpected_crash"] = "UNVERIFIED" if crash is None else "FAIL" if crash else "PASS"
    report["criteria"] = criteria
    report["observer_acceptance"] = ("FAIL" if "FAIL" in criteria.values() else
                                     "PASS" if all(v == "PASS" for v in criteria.values()) else "UNVERIFIED")
    report["operator_reported_criteria"] = ["holds", "native_controls", "no_unexpected_crash"]
    return report


def publish_report(report, run=subprocess.run):
    report = sanitize(report)  # Never publish arbitrary remote fields or full logs.
    def api(path, method="GET", body=None):
        argv = ["gh", "api", "repos/" + REPO + "/" + path, "--method", method]
        options = dict(check=True, capture_output=True, text=True)
        if body is not None:
            argv += ["--input", "-"]
            options["input"] = json.dumps(body)
        return json.loads(run(argv, **options).stdout)
    # A list query returns [] for an absent branch without masking auth/network errors.
    refs = api("git/matching-refs/heads/" + RESULT_BRANCH)
    exact = [r for r in refs if r["ref"] == "refs/heads/" + RESULT_BRANCH]
    parent = exact[0]["object"]["sha"] if exact else None
    tree = {"tree": [{"path": "results/" + report["source_commit"] + ".json",
                      "mode": "100644", "type": "blob", "content": json.dumps(report, indent=2) + "\n"}]}
    if parent:
        tree["base_tree"] = api("git/commits/" + parent)["tree"]["sha"]
    tree_sha = api("git/trees", "POST", tree)["sha"]
    commit = api("git/commits", "POST", {"message": "Arcade Lab evidence " + report["source_commit"],
                 "tree": tree_sha, "parents": [parent] if parent else []})["sha"]
    if parent:
        api("git/refs/heads/" + RESULT_BRANCH, "PATCH", {"sha": commit, "force": False})
    else:
        api("git/refs", "POST", {"ref": "refs/heads/" + RESULT_BRANCH, "sha": commit})


def self_test():
    import contextlib
    import io
    suite = unittest.defaultTestLoader.discover(str(HERE.parent), pattern="test_*.py")
    if suite.countTestCases() < 40:
        raise RuntimeError("Incomplete self-test installation; nothing installed")
    # Fixtures exercise success messages too. Do not expose their simulated
    # LAB READY output as if the user's own Deck had passed preflight.
    with contextlib.redirect_stdout(io.StringIO()):
        return unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful()


def bootstrap():
    # Tests finish before ANY persistent HOME write or key generation.
    if not self_test():
        raise RuntimeError("Self-tests failed; nothing installed")
    destination = HOME / "controller"
    if destination.is_symlink() or any(p.is_symlink() for p in destination.parents):
        raise ValueError("Unsafe lab installation path")
    destination.mkdir(parents=True, exist_ok=True)
    for name in ("deck_installer.py", "deck_updater.py", "package_deck_installer.py", "test_deck_installer.py", "test_lab.py",
                 "test_robot.py", "test_trick_batch.py", "test_wake.py", "manual_install.py", "update_arcade_dev.sh", "restore_stock.sh"):
        installer.safe_path(destination / name)
        shutil.copy2(HERE.parent / name, destination / name)
    installer.safe_path(destination / "lab")
    (destination / "lab").mkdir(exist_ok=True)
    for source in HERE.iterdir():
        if source.suffix in (".py", ".command") and source.is_file() and not source.is_symlink():
            installer.safe_path(destination / "lab" / source.name)
            shutil.copy2(source, destination / "lab" / source.name)
    lab = Lab()
    lab.key()
    if input("Pair with Steam Deck now? [y/N] ").strip().lower() == "y":
        print(lab.pair())
    result = lab.preflight()
    print(json.dumps(result, indent=2))
    if not result["pass"]:
        raise RuntimeError("Lab installed; preflight blocked. No game files changed.")
    print("LAB READY — NO GAME FILES CHANGED")
    print("Actions: " + str(destination / "lab"))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("self-test", "bootstrap", "pair", "preflight", "deploy", "status", "collect", "restore", "trace"))
    parser.add_argument("--host", default="deck@steamdeck.local")
    parser.add_argument("--game")
    parser.add_argument("--log", help="Explicit actual ReSkate.log fallback path; never guessed")
    parser.add_argument("--no-publish", action="store_true")
    for name in ("holds", "native", "no-crash"):
        parser.add_argument("--" + name, choices=("pass", "fail"), help="Operator observation for this session")
    args = parser.parse_args(argv)
    if args.operation == "self-test":
        return 0 if self_test() else 1
    if args.operation == "bootstrap":
        bootstrap()
        return 0
    lab = Lab(host=args.host, game=args.game, log=args.log)
    if args.operation == "pair":
        print(lab.pair())
    elif args.operation == "preflight":
        result = lab.preflight()
        print(json.dumps(result, indent=2))
        print("PREFLIGHT PASS — NO GAME FILES CHANGED" if result["pass"] else "PREFLIGHT FAIL — NO GAME FILES CHANGED")
        return 0 if result["pass"] else 1
    elif args.operation == "trace":
        # Read-only numeric research evidence, separate from observer acceptance.
        print(json.dumps(lab.remote("trace"), indent=2))
    elif args.operation == "collect":
        checks = {name: None if getattr(args, name.replace("-", "_")) is None else
                  getattr(args, name.replace("-", "_")) == "pass" for name in ("holds", "native", "no-crash")}
        if sys.stdin.isatty():
            for name, prompt in (("holds", "Holding each button gave no repeated edges"),
                                 ("native", "Native controls continued normally"),
                                 ("no-crash", "No unexpected crash during this test session")):
                if checks[name] is None:
                    answer = input(prompt + "? [pass/fail/unknown] ").strip().lower()
                    checks[name] = True if answer == "pass" else False if answer == "fail" else None
        path, published, report = lab.collect(args.log, checks["holds"], checks["native"], checks["no-crash"], not args.no_publish)
        print(json.dumps(report, indent=2))
        print("Saved: " + str(path))
        if published:
            print("https://github.com/" + REPO + "/blob/" + RESULT_BRANCH + "/results/" + report["source_commit"] + ".json")
    else:
        result = lab.deploy() if args.operation == "deploy" else lab.remote(args.operation)
        print(json.dumps(result, indent=2))
        if args.operation in ("deploy", "restore"):
            print(args.operation.upper() + " VERIFIED")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        # SSH stderr can contain private paths; never include it in published evidence.
        print("Arcade Lab failed: " + str(error), file=sys.stderr)
        sys.exit(1)
