"""Read-only Deck inspection. Sent in memory over SSH; never installed on Deck.

No installer import, filesystem writes, process signals, or shell execution.
The same functions run against temporary fake Decks in tests.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import re
import socket
import time

SETTINGS = "ReSkateLauncher.settings.json"
BACKUP = "ReSkate.dll.arcade-stock.bak"
RECEIPT = ".arcade-mode-install.json"
# Three native reverts exceeded the old 128-row tail before trial observation.
# Keep correlated input streams bounded together; real loss remains explicit. cat pouch
INPUT_TRACE_LIMIT = 512


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path):
    if not path.exists():
        return {}
    if path.stat().st_size > 1024 * 1024:
        raise ValueError("Metadata too large")
    value = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(value, dict):
        raise ValueError("Invalid metadata")
    return value


def libraries(home, media):
    result = {home / ".local/share/Steam", home / ".steam/steam", home / ".steam/root"}
    for steam in list(result):
        path = steam / "steamapps/libraryfolders.vdf"
        if path.is_file():
            for value in re.findall(r'"path"\s*"([^"\n]+)"', path.read_text(errors="replace")):
                result.add(Path(value.replace("\\\\", "\\")))
    result.update(media.glob("*"))
    result.update(media.glob("*/*"))
    return sorted({p.resolve() for p in result if (p / "steamapps").is_dir()})


def processes(proc=Path("/proc")):
    seen = {"skate": False, "launcher": False, "check_complete": True}
    if not proc.is_dir():
        seen["check_complete"] = False
        return seen
    for process in proc.glob("[0-9]*"):
        try:
            if process.stat().st_uid != os.getuid():
                continue
            argv = (process / "cmdline").read_bytes().decode(errors="replace").split("\0")
            for arg in argv:
                name = arg.replace("\\", "/").rsplit("/", 1)[-1].lower()
                if name == "skate.exe":
                    seen["skate"] = True
                if name == "reskatelauncher.exe":
                    seen["launcher"] = True
        except FileNotFoundError:
            continue  # Process exited during snapshot.
        except OSError:
            seen["check_complete"] = False
    return seen


def snapshot(request, home=None, media=None, process_info=None):
    home = Path.home() if home is None else Path(home)
    media = Path("/run/media") if media is None else Path(media)
    libs = libraries(home, media)
    candidates = set()
    for library in libs:
        common = library / "steamapps/common"
        if common.is_dir():
            for path in common.iterdir():
                if (path / "Skate.exe").is_file() or (path / "ReSkateLauncher.exe").is_file():
                    candidates.add(path.resolve())
    standalone = home / "Games/ReSkate"
    if any((standalone / name).is_file() for name in ("Skate.exe", "ReSkateLauncher.exe")):
        candidates.add(standalone)
    config = home / ".local/share/reskate-arcade/config.json"
    selection_error = None
    if request.get("game"):
        games = [Path(request["game"])]
    elif config.exists() or config.is_symlink():
        # During migration both copies remain present. Never silently fall back
        # to Steam's copy when the selected installation is missing or invalid.
        try:
            if any(p.is_symlink() for p in (config, *config.parents)) or not config.is_file():
                raise ValueError("Unsafe configured installation")
            value = read_json(config).get("game")
            if not isinstance(value, str) or not Path(value).is_absolute() or Path(value) not in candidates:
                raise ValueError("Configured installation is missing or outside discovered locations")
            games = [Path(value)]
        except (OSError, ValueError) as error:
            games = []
            selection_error = str(error)
    else:
        games = sorted(candidates)
    proc = processes() if process_info is None else process_info
    result = {"ssh_connected": True, "user": __import__("pwd").getpwuid(os.getuid()).pw_name,
              "host": socket.gethostname(), "steam_libraries": [str(p) for p in libs],
              "games": [str(p) for p in games], "processes": proc, "pass": False}
    if len(games) != 1:
        result["reason"] = selection_error or "Select exactly one game with --game"
        return result
    game = games[0]
    names = ("Skate.exe", "ReSkateLauncher.exe", "ReSkate.dll", SETTINGS, BACKUP, RECEIPT,
             ".arcade-mode-transaction.json", ".arcade-mode-install.lock")
    safe = all(not p.is_symlink() for p in (game, *game.parents))
    safe = safe and all(not (game / n).is_symlink() and
                       (not (game / n).exists() or (game / n).is_file()) for n in names)
    result.update(game=str(game), safe_paths=safe,
                  files={n: (game / n).is_file() for n in names[:3]},
                  backup_present=(game / BACKUP).is_file(),
                  receipt_present=(game / RECEIPT).is_file(),
                  config_present=(home / ".local/share/reskate-arcade/config.json").is_file(),
                  recovery_pending=(game / ".arcade-mode-transaction.json").exists())
    if not safe or not game.is_dir():
        result["reason"] = "Unsafe or missing installation path"
        return result
    try:
        receipt = read_json(game / RECEIPT)
        result["receipt_readable"] = True
    except (OSError, ValueError):
        receipt = {}
        result["receipt_readable"] = False
    try:
        settings = read_json(game / SETTINGS)
        result["settings_readable"] = True
    except (OSError, ValueError):
        settings = {}
        result["settings_readable"] = False
    dll = game / "ReSkate.dll"
    free = os.statvfs(game)
    required = 8 * (request.get("dll_size", 0) + (dll.stat().st_size if dll.is_file() else 0)) + 16 * 1024 * 1024
    result.update(dll_sha256=sha(dll) if dll.is_file() else None,
                  backup_sha256=sha(game / BACKUP) if (game / BACKUP).is_file() else None,
                  installed_commit=receipt.get("commit"), active=receipt.get("active", False),
                  updates=settings.get("updates", True),
                  free_bytes=free.f_bavail * free.f_frsize, required_bytes=required)
    result["pass"] = (all(result["files"].values()) and proc["check_complete"]
                      and not proc["skate"] and not proc["launcher"]
                      and result["free_bytes"] >= required and not result["recovery_pending"]
                      and result["settings_readable"] and result["receipt_readable"]
                      and isinstance(result["updates"], bool))
    if not result["pass"]:
        result["reason"] = "Missing files, running/unknown processes, recovery pending, settings or space check failed"
    return result


def evidence(request, **kwargs):
    status = snapshot(request, **kwargs)
    if not status.get("safe_paths") or not all(status.get("files", {}).values()):
        raise ValueError("Cannot collect from an unsafe/incomplete installation")
    game = Path(status["game"])
    receipt = read_json(game / RECEIPT)
    commit, dll_hash = receipt.get("commit", ""), receipt.get("dll_sha256", "")
    if not isinstance(commit, str) or not re.fullmatch("[0-9a-f]{40}", commit):
        raise ValueError("No valid installed source receipt")
    if not isinstance(dll_hash, str) or not re.fullmatch("[0-9a-f]{64}", dll_hash):
        raise ValueError("No valid installed DLL receipt")
    identity = receipt.get("installation_id", "")
    if not isinstance(identity, str) or not re.fullmatch("[0-9a-f]{32}", identity):
        raise ValueError("Deploy with Arcade Lab before collecting session-bound evidence")
    active = receipt.get("active") is True
    baseline_log = Path(receipt.get("log_path", str(game / "logs/ReSkate.log")))
    log = Path(request["log"]) if request.get("log") else baseline_log
    if log.name != "ReSkate.log" or any(p.is_symlink() for p in (log, *log.parents)):
        raise ValueError("Expected a regular ReSkate.log path")
    text = ""
    fresh = False
    if active and log.is_file() and log.stat().st_size <= 16 * 1024 * 1024:
        fresh = (log.stat().st_mtime > receipt.get("installed_at", float("inf"))
                 and sha(log) != receipt.get("previous_log_sha256")
                 and status["dll_sha256"] == dll_hash)
        # An appended old log must not reattribute an old session to a new DLL.
        prefix_size = receipt.get("previous_log_prefix_size", 0)
        if prefix_size:
            with log.open("rb") as stream:
                same_prefix = hashlib.sha256(stream.read(prefix_size)).hexdigest() == receipt.get("previous_log_prefix_sha256")
            fresh = fresh and not same_prefix
        if log != baseline_log:
            fresh = False  # No deployment baseline exists for this override yet.
        if fresh:
            text = log.read_text(errors="replace")
    # Use only the most recent ready event, never union buttons across launches.
    start = text.rfind("Arcade Mode ready")
    session = text[start:] if start >= 0 else ""
    # logging::event renders underscores as spaces and scalar members as key=value.
    # Raw JSON exists only in its optional trace-level "Native event" line.
    initialized = bool(re.search(r'Arcade mode initialized: active=true\b', session)
                       or re.search(r'"event"\s*:\s*"arcade_mode_initialized"\s*,\s*"active"\s*:\s*true', session))
    buttons = {b: bool(re.search(r"ARCADE MODE DEV: " + b + r" press #[1-9][0-9]* seen on ReSkate client tick\.", session)) for b in "ABXY"}
    crashed = "ReSkate unhandled exception" in session
    restored = (not active and status["dll_sha256"] == receipt.get("stock_sha256")
                and status["backup_sha256"] == receipt.get("stock_sha256")
                and receipt.get("restored_sha256") == receipt.get("stock_sha256"))
    updates_restored = restored and status["updates"] == receipt.get("updates_before")
    return {"schema": 1, "source_commit": commit, "installed_dll_sha256": dll_hash,
            "installation_id": identity, "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "reskate_process_seen": status["processes"]["skate"],
            "arcade_mode_initialized": initialized, "arcade_mode_ready": bool(session),
            "buttons": buttons, "unexpected_crash": True if crashed else None,
            "fresh_log": fresh, "stock_restore": "PASS" if restored else "UNVERIFIED",
            "updater_restore": "PASS" if updates_restored else "UNVERIFIED"}


def parse_native_captures(text):
    """Extract only bounded numeric diagnostics, never arbitrary log excerpts."""
    captures = []
    current = None
    for line in text.splitlines():
        begin = re.search(r'ARCADE NATIVE CAPTURE: id=([0-9]{1,20}) begin=true\.$', line)
        if begin:
            current = {"id": int(begin[1]), "complete": False, "samples": []}
            captures.append(current)
            captures = captures[-8:]
            continue
        if current is None:
            continue
        end = re.search(r'ARCADE NATIVE CAPTURE: id=([0-9]{1,20}) end=true samples=([0-9]{1,3})\.$', line)
        if end and int(end[1]) == current["id"]:
            current["complete"] = int(end[2]) == len(current["samples"])
            current = None
            continue
        sample = re.search(r'ARCADE NATIVE SAMPLE: id=([0-9]{1,20}) ms=([0-9]{1,4}) valid=(true|false) state=([0-9]{1,10}) crouch_mode=([0-9]{1,10}) words=([0-9a-f]{8}(?:,[0-9a-f]{8}){7})\.$', line)
        if (sample and int(sample[1]) == current["id"] and int(sample[2]) < 5000
                and len(current["samples"]) < 128
                and int(sample[4]) <= 0xffffffff and int(sample[5]) <= 0xffffffff):
            valid = sample[3] == "true"
            current["samples"].append({"ms": int(sample[2]), "valid": valid,
                "state": int(sample[4]) if valid else 0,
                "crouch_mode": int(sample[5]) if valid else 0,
                "words": [int(x, 16) for x in sample[6].split(",")] if valid else [0]*8})
    return captures


def parse_combo_samples(text):
    """Numeric allowlist only; no unrelated text or memory addresses returned."""
    samples, events, balance, failures, bails, reverts, dropped = [], [], [], [], [], [], 0
    for line in text.splitlines():
        revert = re.search(r'ARCADE REVERT LINK: ms=(\d{1,20}) sequence=(\d{1,20}) labels=(4|8|16777216) accepted=(true|false) remaining_ms=(\d{1,5})\.$', line)
        if revert:
            ms, sequence, labels, remaining = (int(revert[i]) for i in (1, 2, 3, 5))
            if max(ms, sequence) <= 0xffffffffffffffff and sequence and remaining <= 10000:
                reverts.append(dict(ms=ms, sequence=sequence, labels=labels,
                                    accepted=revert[4] == 'true', remaining_ms=remaining))
                if len(reverts) > 128:
                    reverts.pop(0)
                    dropped += 1
        failure = re.search(r'ARCADE BALANCE FAILURE: ms=([0-9]{1,20}) kind=([12]) request=([0-9]{1,20})\.$', line)
        if failure and max(int(failure[1]), int(failure[3])) <= 0xffffffffffffffff:
            failures.append(dict(ms=int(failure[1]), kind=int(failure[2]), request=int(failure[3])))
            failures = failures[-128:]
        bail = re.search(r'ARCADE BALANCE BAIL: ms=([0-9]{1,20}) request=([0-9]{1,20}) result=([0-8])\.$', line)
        if bail and max(int(bail[1]), int(bail[2])) <= 0xffffffffffffffff:
            bails.append(dict(ms=int(bail[1]), request=int(bail[2]), result=int(bail[3])))
            bails = bails[-128:]
        event = re.search(r'ARCADE COMBO EVENT: ms=([0-9]{1,20}) pose=([0-7]) active=(true|false) failed=(true|false) fatigue=([0-9.eE+-]{1,24}) grind=([0-9.eE+-]{1,24}) manual=([0-9.eE+-]{1,24}) native_failure_verified=false\.$', line)
        if event and int(event[1]) <= 0xffffffffffffffff:
            try:
                fatigue = [float(event[i]) for i in (5, 6, 7)]
                if all(0 <= value <= 1 for value in fatigue):
                    events.append({"ms": int(event[1]), "pose": int(event[2]),
                        "active": event[3] == "true", "failed": event[4] == "true",
                        "fatigue": fatigue[0], "grind_fatigue": fatigue[1],
                        "manual_fatigue": fatigue[2], "native_failure_verified": False})
                    events = events[-128:]
            except ValueError:
                pass
        meter = re.search(r'ARCADE BALANCE SAMPLE: ms=([0-9]{1,20}) kind=([12]) x=([0-9.eE+-]{1,24}) v=([0-9.eE+-]{1,24}) input=([0-9.eE+-]{1,24}) difficulty=([0-9.eE+-]{1,24}) raw_x=(-?[0-9]{1,5}) raw_y=(-?[0-9]{1,5})\.$', line)
        if meter and int(meter[1]) <= 0xffffffffffffffff:
            try:
                x, velocity, correction, difficulty = (float(meter[i]) for i in (3, 4, 5, 6))
                raw_x, raw_y = int(meter[7]), int(meter[8])
                if (abs(x) <= 1 and abs(velocity) <= 100 and abs(correction) <= 1 and
                        0 <= difficulty <= 1 and -32768 <= raw_x <= 32767 and -32768 <= raw_y <= 32767):
                    balance.append({"ms": int(meter[1]), "kind": int(meter[2]), "x": x,
                        "velocity": velocity, "correction": correction, "difficulty": difficulty,
                        "raw_x": raw_x, "raw_y": raw_y})
                    balance = balance[-128:]
            except ValueError:
                pass
        gap = re.search(r'ARCADE COMBO GAP: dropped=([0-9]{1,20})\.$', line)
        if gap:
            dropped = min(0xffffffffffffffff, dropped + int(gap[1]))
        match = re.search(r'ARCADE COMBO SAMPLE: ms=([0-9]{1,20}) state=([0-9]{1,10}) mode=([0-9]{1,10}) words=([0-9a-f]{8}(?:,[0-9a-f]{8}){7})\.$', line)
        if match and int(match[1]) <= 0xffffffffffffffff and max(int(match[2]), int(match[3])) <= 0xffffffff:
            samples.append({"ms": int(match[1]), "state": int(match[2]), "mode": int(match[3]),
                            "words": [int(word, 16) for word in match[4].split(',')]})
            samples = samples[-128:]
    return {"samples": samples, "events": events, "balance": balance,
            "failure_requests": failures, "bail_results": bails, "revert_links": reverts,
            "dropped": dropped, "native_event_identity": "UNVERIFIED"}


def parse_score_samples(text):
    fields, samples, nodes, values = [], [], [], []
    labels, labels_dropped = [], 0
    for line in text.splitlines():
        label = re.search(r'ARCADE SCORE LABEL: ms=([0-9]{1,20}) path=([0-9a-f]{8}(?:\.[0-9a-f]{8}){0,3}) id=([0-9]{1,2})\.$', line)
        if label and int(label[1]) <= 0xffffffffffffffff and 1 <= int(label[3]) <= 26:
            labels.append({"ms": int(label[1]), "path": label[2], "id": int(label[3])})
            if len(labels) > 128:
                labels.pop(0)
                labels_dropped += 1
        field = re.search(r'ARCADE SCORE FIELD: hash=([0-9a-f]{8}) kind=([0-9]{1,2}) size=([0-9]{1,5})\.$', line)
        if field and int(field[2]) <= 31 and int(field[3]) <= 65535:
            fields.append({"hash": field[1], "kind": int(field[2]), "size": int(field[3])})
            fields = fields[-128:]
        sample = re.search(r'ARCADE SCORE SAMPLE: ms=([0-9]{1,20}) hash=([0-9a-f]{8}) value=([0-4])\.$', line)
        if sample and int(sample[1]) <= 0xffffffffffffffff:
            samples.append({"ms": int(sample[1]), "hash": sample[2], "value": int(sample[3])})
            samples = samples[-128:]
        node = re.search(r'ARCADE SCORE NODE: path=([0-9a-f]{8}(?:\.[0-9a-f]{8}){0,3}) kind=([0-9]{1,2}) size=([0-9]{1,5})\.$', line)
        if node and int(node[2]) <= 31 and int(node[3]) <= 65535:
            nodes.append({"path": node[1], "kind": int(node[2]), "size": int(node[3])})
            nodes = nodes[-256:]
        value = re.search(r'ARCADE SCORE VALUE: ms=([0-9]{1,20}) path=([0-9a-f]{8}(?:\.[0-9a-f]{8}){0,3}) value=([0-9]{1,10})\.$', line)
        if value and int(value[1]) <= 0xffffffffffffffff and int(value[3]) <= 0xffffffff:
            values.append({"ms": int(value[1]), "path": value[2], "value": int(value[3])})
            values = values[-128:]
    return {"fields": fields, "samples": samples, "nodes": nodes, "values": values,
            "labels": labels, "labels_dropped": labels_dropped,
            "limit_reached": any(line.endswith("ARCADE SCORE LIMIT: reached=true.") for line in text.splitlines()),
            "event_identity": "UNVERIFIED"}


def parse_pad_samples(text):
    """Only numeric controller fields from the game-facing adapter; bounded tail."""
    samples, sources, dropped = [], [], 0
    for line in text.splitlines():
        gap = re.search(r'ARCADE PAD GAP: dropped=([0-9]{1,20})\.$', line)
        if gap and int(gap[1]) <= 0xffffffffffffffff:
            dropped = min(0xffffffffffffffff, dropped + int(gap[1]))
        source = re.search(r'ARCADE PAD SOURCE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) user=([0-3]) buttons=([0-9]{1,5}) lt=([0-9]{1,3}) rt=([0-9]{1,3})\.$', line)
        if source:
            values = [int(v) for v in source.groups()]
            if max(values[:2]) <= 0xffffffffffffffff and values[3] <= 65535 and max(values[4:]) <= 255:
                sources.append(dict(zip(('seq', 'ms', 'user', 'buttons', 'lt', 'rt'), values)))
                if len(sources) > INPUT_TRACE_LIMIT:
                    sources.pop(0)
                    dropped = min(0xffffffffffffffff, dropped + 1)
        row = re.search(r'ARCADE PAD SAMPLE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) user=([0-3]) buttons=([0-9]{1,5}) lt=([0-9]{1,3}) rt=([0-9]{1,3}) output=([0-9]{1,5}) phase=([0-3])\.$', line)
        if not row:
            continue
        values = [int(v) for v in row.groups()]
        if max(values[:2]) > 0xffffffffffffffff or max(values[3], values[6]) > 65535 or max(values[4:6]) > 255:
            continue
        samples.append(dict(zip(('seq', 'ms', 'user', 'buttons', 'lt', 'rt', 'output', 'phase'), values)))
        if len(samples) > INPUT_TRACE_LIMIT:
            samples.pop(0)
            dropped = min(0xffffffffffffffff, dropped + 1)
    return {'samples': samples, 'source_samples': sources, 'dropped': dropped}


def parse_push_samples(text):
    """Bounded numeric speed evidence; never return arbitrary log text."""
    samples = []
    for line in text.splitlines():
        match = re.search(r'ARCADE PUSH OBSERVATION: seq=([0-9]{1,20}) ms=([0-9]{1,20}) valid=(true|false) eligible=(true|false) speed=([0-9.eE+-]{1,24}) contacts=([0-9]{1,5})\.$', line)
        if not match or max(int(match[1]), int(match[2])) > 0xffffffffffffffff:
            continue
        try:
            speed, contacts = float(match[5]), int(match[6])
            if 0 <= speed < 1500 and contacts in (0, 0x4000, 0x8000, 0xc000):
                samples.append({'seq': int(match[1]), 'ms': int(match[2]),
                                'valid': match[3] == 'true', 'eligible': match[4] == 'true',
                                'speed': speed, 'contacts': contacts})
                samples = samples[-128:]
        except ValueError:
            pass
    return samples


def parse_flip_observations(text):
    # Changed physics words can repeat a selected flip during a native bail.
    # Cover the bounded12s trial plus10s recovery at60Hz without losing its start.
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE FLIP OBSERVATION: ms=([0-9]{1,20}) valid=(true|false) id=(-?[0-9]{1,2})\.$', line)
        if not row or int(row[1]) > 0xffffffffffffffff or not -1 <= int(row[3]) <= 63:
            continue
        samples.append({'ms': int(row[1]), 'valid': row[2] == 'true', 'id': int(row[3])})
        if len(samples) > 2048:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_stick_samples(text):
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE STICK SAMPLE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) tick=([0-9]{1,20}) user=([0-3]) x=(-?[0-9]{1,5}) y=(-?[0-9]{1,5}) phase=([0-3])\.$', line)
        if not row:
            continue
        seq, ms, tick, user, x, y, phase = map(int, row.groups())
        if max(seq, ms, tick) > 0xffffffffffffffff or not (-32768 <= x <= 32767 and -32768 <= y <= 32767):
            continue
        samples.append(dict(seq=seq, ms=ms, tick=tick, user=user, x=x, y=y, phase=phase))
        if len(samples) > INPUT_TRACE_LIMIT:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_playback_samples(text):
    """Bounded numeric cadence and packet evidence, joined to stick rows by seq."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE PLAYBACK SAMPLE: seq=([0-9]{1,20}) poll=([0-9]{1,20}) tick=([0-9]{1,20}) mode=([0-2]) input_packet=([0-9]{1,10}) output_packet=([0-9]{1,10})\.$', line)
        if not row:
            continue
        seq, poll, tick, mode, input_packet, output_packet = map(int, row.groups())
        if max(seq, poll, tick) > 0xffffffffffffffff or max(input_packet, output_packet) > 0xffffffff:
            continue
        samples.append(dict(seq=seq, poll=poll, tick=tick, mode=mode,
                            input_packet=input_packet, output_packet=output_packet))
        if len(samples) > INPUT_TRACE_LIMIT:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_action_samples(text):
    """Numeric input-dispatch evidence only; seq joins the independent stick row."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE ACTION SAMPLE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) tick=([0-9]{1,20}) user=([0-3]) mode=([0-5]) requested=(true|false) kind=([0-5]) trick=([0-9]{1,2}) result=([0-5]) owned=(true|false) lx=(-?[0-9]{1,5}) ly=(-?[0-9]{1,5}) lt=([0-9]{1,3}) rt=([0-9]{1,3}) packet=([0-9]{1,10})\.$', line)
        if not row:
            continue
        names = ('seq', 'ms', 'tick', 'user', 'mode', 'requested', 'kind', 'trick',
                 'result', 'owned', 'lx', 'ly', 'lt', 'rt', 'packet')
        sample = {key: value == 'true' if key in ('requested', 'owned') else int(value)
                  for key, value in zip(names, row.groups())}
        # Kind5 is shoulder rotation. result is DispatchResult, never native success.
        # ArcadeTrick::darkslide is53; rotation does not fabricate a trick identifier.
        if (max(sample['seq'], sample['ms'], sample['tick']) > 0xffffffffffffffff or
                sample['trick'] > 53 or sample['packet'] > 0xffffffff or
                not (-32768 <= sample['lx'] <= 32767 and -32768 <= sample['ly'] <= 32767) or
                max(sample['lt'], sample['rt']) > 255):
            continue
        samples.append(sample)
        if len(samples) > INPUT_TRACE_LIMIT:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_score_edges(text):
    """Numeric label appearances; slot roles and native action identity remain unverified."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE SCORE EDGE: seq=([0-9]{1,20}) observed=([0-9]{1,20}) sampled=([0-9]{1,20}) labels=([0-9]{1,8}) uncertainty=([0-5])\.$', line)
        if not row:
            continue
        seq, observed, sampled, labels, uncertainty = map(int, row.groups())
        if max(seq, observed, sampled) > 0xffffffffffffffff or labels > 67108863:
            continue
        samples.append(dict(seq=seq, observed=observed, sampled=sampled, labels=labels, uncertainty=uncertainty))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_manual_history(text):
    """Bounded presentation diagnostics only; counts are not native occurrences.

    Producer comparison is against the preceding complete50ms snapshot, while
    unchanged diagnostic rows may be logged only once per second. Do not infer
    missing game snapshots or recompute its deltas from adjacent log rows.
    """
    counts = ('rows', 'manual', 'nose', 'distinct', 'manual_distinct', 'nose_distinct',
              'manual_added', 'manual_removed', 'nose_added', 'nose_removed', 'duplicates', 'relabels')
    statuses = ('baseline', 'complete', 'gap', 'clock_reset', 'model_changed', 'invalid', 'fallback')
    pattern = (r'ARCADE MANUAL HISTORY: seq=([0-9]{1,20}) ms=([0-9]{1,20}) status=([0-6]) '
               r'available=(true|false) comparable=(true|false) ' +
               ' '.join(key + r'=([0-9]{1,2})' for key in counts) + r'\.$')
    samples, dropped, malformed, sequence_gaps, sequence_missing = [], 0, 0, 0, 0
    previous_seq = previous_ms = None
    for line in text.splitlines():
        if 'ARCADE MANUAL HISTORY:' not in line:
            continue
        match = re.search(pattern, line)
        if not match:
            malformed += 1
            continue
        seq, ms, status = map(int, match.groups()[:3])
        available, comparable = (value == 'true' for value in match.groups()[3:5])
        row = dict(zip(counts, map(int, match.groups()[5:])))
        deltas = [row[k] for k in ('manual_added', 'manual_removed', 'nose_added', 'nose_removed', 'relabels')]
        if (not 0 < seq <= 0xffffffffffffffff or ms > 0xffffffffffffffff or max(row.values()) > 16 or
                available != (status <= 4) or comparable != (status == 1) or
                (not available and any(row.values())) or (not comparable and any(deltas)) or
                row['rows'] != row['distinct'] + row['duplicates'] or
                row['manual'] + row['nose'] > row['rows'] or
                row['manual_distinct'] + row['nose_distinct'] > row['distinct'] or
                row['manual_distinct'] > row['manual'] or row['nose_distinct'] > row['nose'] or
                row['manual'] + row['nose'] - row['manual_distinct'] - row['nose_distinct'] > row['duplicates'] or
                row['manual_added'] > row['manual_distinct'] or row['nose_added'] > row['nose_distinct'] or
                row['relabels'] > row['distinct'] or
                (previous_seq is not None and seq <= previous_seq) or
                (previous_ms is not None and ms < previous_ms and status != 3)):
            malformed += 1
            continue
        if previous_seq is not None and seq > previous_seq + 1:
            sequence_gaps += 1
            sequence_missing += seq - previous_seq - 1
        previous_seq, previous_ms = seq, ms
        # Invalid/fallback counts are absent measurements, not observed zeroes.
        samples.append(dict(seq=seq, ms=ms, status=statuses[status], available=available,
                            comparable=comparable, **{key: row[key] if available else None for key in counts}))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return dict(samples=samples, dropped=dropped, malformed=malformed, sequence_gaps=sequence_gaps,
                sequence_missing=sequence_missing, presence='present' if samples else 'invalid' if malformed else 'missing',
                native_identity='UNVERIFIED', authoritative=False, capture_completeness='UNVERIFIED')


def parse_score_presence(text):
    """Bounded reader diagnostics; split components are not named native outcomes."""
    samples, dropped = [], 0
    for line in text.splitlines():
        match = re.search(r'ARCADE SCORE PRESENCE: ms=(\d{1,20}) source=([0-2]) rows=(\d{1,2}) stage=([0-5]) parts=(\d{1,2}) labels=(\d{1,8})\.$', line)
        if not match:
            continue
        ms, source, rows, stage, parts, labels = map(int, match.groups())
        if ms > 0xffffffffffffffff or rows > 16 or parts > 31 or labels > 67108863:
            continue
        samples.append(dict(ms=ms, source=source, rows=rows, stage=stage, parts=parts, labels=labels))
        if len(samples) > 256:
            samples.pop(0)
            dropped += 1
    return dict(samples=samples, dropped=dropped, native_identity='UNVERIFIED',
                manual_history=parse_manual_history(text))


def parse_score_numbers(text):
    """Native HUD presentation values, never proof of ownership, a bank, or a trick."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE SCORE NUMBERS: ms=([0-9]{1,20}) sampled=([0-9]{1,20}) score_valid=(true|false) score=([0-9]{1,20}) multiplier_valid=(true|false) multiplier=([0-9]{1,20}) owner_verified=false\.$', line)
        if not row:
            continue
        ms, sampled, score_valid, score, multiplier_valid, multiplier = row.groups()
        ms, sampled, score, multiplier = map(int, (ms, sampled, score, multiplier))
        if (max(ms, sampled, score, multiplier) > 0xffffffffffffffff or sampled > ms or
                (score_valid == 'false' and score != 0) or
                (multiplier_valid == 'false' and multiplier != 0) or
                (multiplier_valid == 'true' and multiplier == 0)):
            continue
        samples.append(dict(ms=ms, sampled=sampled, score_valid=score_valid == 'true', score=score,
                            multiplier_valid=multiplier_valid == 'true', multiplier=multiplier,
                            owner_verified=False, fresh=ms - sampled <= 150))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_native_score_details(text):
    """Optional seq-keyed scalar observations; missing values never mean zero.

    These rows do not identify a native occurrence or owner. A consumer must
    retain missing detail for any capture without a matching row as unverified;
    this parser does not join, deduplicate, or turn diagnostics into score proof.
    """
    samples, dropped, malformed = [], 0, 0
    statuses = {1: 'schema_unavailable', 2: 'read_failed', 3: 'invalid_value',
                4: 'absent', 5: 'available'}
    pattern = (r'ARCADE NATIVE SCORE DETAIL: seq=([0-9]{1,20}) scalar_status=([1235]) '
               r'category_status=([1245]) sequence_kind=([0-9]{1,10}) '
               r'action_score=([0-9.eE+-]{1,32}) base_score=([0-9.eE+-]{1,32}) '
               r'landing=([0-9.eE+-]{1,32}) current_category=([0-9]{1,10}) authoritative=false\.$')
    for line in text.splitlines():
        if 'ARCADE NATIVE SCORE DETAIL:' not in line:
            continue
        row = re.search(pattern, line)
        if not row:
            malformed += 1
            continue
        seq, scalar_status, category_status, kind, action, base, landing, category = row.groups()
        seq, scalar_status, category_status, kind, category = map(int, (seq, scalar_status, category_status, kind, category))
        try:
            action, base, landing = map(float, (action, base, landing))
        except ValueError:
            malformed += 1
            continue
        if (not 0 < seq <= 0xffffffffffffffff or max(kind, category) > 0xffffffff or
                not all(math.isfinite(value) for value in (action, base, landing)) or
                not all(0 <= value <= 2147483647 and value.is_integer() for value in (action, base)) or
                abs(landing) >= 2**128 or
                (scalar_status != 5 and (kind or action or base or landing)) or
                (category_status != 5 and category)):
            malformed += 1
            continue
        samples.append(dict(seq=seq, scalar_status=statuses[scalar_status], category_status=statuses[category_status],
                            sequence_kind=kind if scalar_status == 5 else None,
                            action_score=action if scalar_status == 5 else None,
                            base_score=base if scalar_status == 5 else None,
                            landing=landing if scalar_status == 5 else None,
                            current_category=category if category_status == 5 else None,
                            authoritative=False, verification='UNVERIFIED'))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return dict(details=samples, detail_dropped=dropped, detail_malformed=malformed,
                detail_presence='present' if samples else 'invalid' if malformed else 'missing',
                score_semantics='UNVERIFIED', payload_owner='UNVERIFIED')


def parse_native_score_health(text):
    """Sampled health and cumulative hook skips, never proof of complete capture."""
    samples, dropped, malformed = [], 0, 0
    pattern = (r'ARCADE NATIVE SCORE HEALTH: ms=([0-9]{1,20}) epoch=([0-9]{1,20}) '
               r'active=(true|false) bound=(true|false) route=([1-4]) '
               r'unbound=([0-9]{1,20}) expired=([0-9]{1,20}) changed=([0-9]{1,20}) overflow=(true|false)\.$')
    for line in text.splitlines():
        if 'ARCADE NATIVE SCORE HEALTH:' not in line:
            continue
        match = re.search(pattern, line)
        if not match:
            malformed += 1
            continue
        ms, epoch, active, bound, route, unbound, expired, changed, overflow = match.groups()
        ms, epoch, route, unbound, expired, changed = map(int, (ms, epoch, route, unbound, expired, changed))
        if (not epoch or max(ms, epoch, unbound, expired, changed) > 0xffffffffffffffff or
                (bound == 'true' and active != 'true')):
            malformed += 1
            continue
        samples.append(dict(ms=ms, epoch=epoch, active=active == 'true', bound=bound == 'true',
            route=route, unbound=unbound, expired=expired, changed=changed, overflow=overflow == 'true'))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return dict(health=samples, health_dropped=dropped, health_malformed=malformed,
                capture_completeness='UNVERIFIED')


def parse_native_score_schema(text):
    """Newest bounded declaration pair only; never owner or event identity proof.

    Member order and the terminal footer are part of the wire contract. An
    incomplete newer snapshot never falls back to an older pair of fields.
    """
    field_keys = ('seq', 'epoch', 'role', 'index', 'type_hash', 'type_size',
                  'field_hash', 'kind', 'size', 'ref_type_hash')
    footer_keys = ('seq', 'ms', 'epoch', 'status', 'line_count', 'sequence_count', 'emitted', 'incomplete')
    def pattern(label, keys, suffix=''):
        return re.compile(re.escape(label) + ' '.join(key + r'=([0-9]{1,20})' for key in keys) + suffix + r'\.$')
    field_pattern = pattern('ARCADE NATIVE SCORE SCHEMA FIELD: ', field_keys)
    footer_pattern = pattern('ARCADE NATIVE SCORE SCHEMA: ', footer_keys, ' owner_verified=false')
    states, malformed, dropped, unassigned, newest, received = {}, 0, 0, 0, 0, 0
    hashes = {1: 0xf95bfe33, 2: 1606121107}
    for line in text.splitlines():
        if 'ARCADE NATIVE SCORE SCHEMA' not in line:
            continue
        match, keys = field_pattern.search(line), field_keys
        if not match:
            match, keys = footer_pattern.search(line), footer_keys
        if not match:
            malformed += 1
            seq_match = re.search(r'ARCADE NATIVE SCORE SCHEMA(?: FIELD)?: seq=([0-9]{1,20}) ', line)
            seq = int(seq_match[1]) if seq_match else 0
            if not 1 <= seq <= 24:
                unassigned += 1
                continue
            state = states.setdefault(seq, dict(fields=[], footer=None, bad=False, epoch=None))
            state['bad'] = True
            newest = max(newest, seq)
            continue
        row = dict(zip(keys, map(int, match.groups())))
        seq = row['seq']
        if not 1 <= seq <= 24:
            malformed += 1
            unassigned += 1
            continue
        state = states.setdefault(seq, dict(fields=[], footer=None, bad=False, epoch=None))
        invalid = (seq < newest or not 0 < row['epoch'] <= 0xffffffffffffffff or
                   state['footer'] is not None or
                   (state['epoch'] is not None and state['epoch'] != row['epoch']))
        newest = max(newest, seq)
        state['epoch'] = row['epoch']
        if keys is field_keys:
            received += 1
            if received > 24 * 192 or len(state['fields']) >= 192:
                dropped += 1
                state['bad'] = True
                continue
            role, index = row['role'], row['index']
            prior = state['fields'][-1] if state['fields'] else None
            invalid |= (role not in hashes or row['type_hash'] != hashes.get(role) or
                        not 0 <= index < 96 or not 1 <= row['type_size'] <= 65535 or
                        not 1 <= row['size'] <= row['type_size'] or not 1 <= row['kind'] <= 28 or
                        max(row['field_hash'], row['ref_type_hash']) > 0xffffffff or
                        (prior is None and (role, index) != (1, 0)) or
                        (prior is not None and (role, index) not in (
                            (prior['role'], prior['index'] + 1), (2, 0))) or
                        (prior is not None and role == 2 and index == 0 and prior['role'] != 1))
            same_role = [value for value in state['fields'] if value['role'] == role]
            invalid |= any(value['field_hash'] == row['field_hash'] or
                           value['type_size'] != row['type_size'] for value in same_role)
            if not invalid:
                state['fields'].append(row)
        else:
            invalid |= (row['ms'] > 0xffffffffffffffff or row['status'] > 4 or
                        max(row['line_count'], row['sequence_count']) > 96 or row['emitted'] > 192 or
                        row['incomplete'] > seq)
            if not invalid:
                state['footer'] = row
        if invalid:
            malformed += 1
            state['bad'] = True
    incomplete = 0
    for state in states.values():
        footer = state['footer']
        counts = [sum(row['role'] == role for row in state['fields']) for role in (1, 2)]
        state['complete'] = (not state['bad'] and footer is not None and footer['status'] == 0 and
                             1 <= counts[0] <= 96 and 1 <= counts[1] <= 96 and
                             counts == [footer['line_count'], footer['sequence_count']] and
                             footer['emitted'] == sum(counts))
        incomplete += not state['complete']
    latest = states.get(newest)
    valid = latest is not None and latest['complete'] and not unassigned and not dropped
    snapshot = None
    if valid:
        snapshot = dict(footer=latest['footer'], fields=latest['fields'], owner_verified=False,
                        native_event_identity='UNVERIFIED')
    return dict(schema_snapshot=snapshot, schema_latest_footer=latest['footer'] if latest else None,
                schema_status='COMPLETE' if valid else 'UNVERIFIED' if states or malformed else 'MISSING',
                schema_malformed=malformed, schema_dropped=dropped, schema_incomplete=incomplete,
                schema_snapshots_seen=len(states), schema_superseded=max(0, len(states) - 1),
                schema_evidence_gap=bool(malformed or dropped or incomplete or
                                        (latest and latest['footer'] and latest['footer']['incomplete'])),
                schema_owner='UNVERIFIED')


def parse_native_score_binder(text):
    """Bounded cumulative mapping diagnostics; never owner or freshness proof."""
    start = text.rfind('Arcade Mode ready')
    if start >= 0:
        text = text[start:]
    counters = ('contract_rejected', 'mapping_unreadable', 'count_rejected', 'layout_rejected',
                'value_mismatch', 'relationship_matches', 'unverified', 'stale')
    keys = ('ms', 'epoch', 'active', 'prepared', 'attached', 'route', *counters, 'overflow')
    booleans = {'active', 'prepared', 'attached', 'overflow'}
    pattern = re.compile(r'ARCADE NATIVE SCORE BINDER: ' + ' '.join(
        key + ('=(true|false)' if key in booleans else r'=([0-9]{1,20})') for key in keys) +
        r' owner_verified=false\.$')
    rows, previous, group, latest = [], {}, None, None
    malformed = dropped = incomplete = received = 0
    for line in text.splitlines():
        if 'ARCADE NATIVE SCORE BINDER:' not in line:
            continue
        received += 1
        if received > 128:
            dropped += 1
            continue
        match = pattern.search(line)
        if not match:
            malformed += 1
            continue
        row = {key: value == 'true' if key in booleans else int(value)
               for key, value in zip(keys, match.groups())}
        if (any(row[key] > 0xffffffffffffffff for key in keys if key not in booleans) or
                not row['epoch'] or row['route'] not in (1, 2, 3, 4) or
                (row['prepared'] and not row['attached'])):
            malformed += 1
            continue
        current = (row['ms'], row['epoch'])
        if group != current:
            if rows and len(rows) != 4:
                incomplete += 1
            if group and (current[0] < group[0] or current[1] < group[1]):
                malformed += 1
            rows, group = [], current
        prior = previous.get(row['route'])
        if (row['route'] != len(rows) + 1 or
                (prior and any(row[key] < prior[key] for key in counters)) or
                (rows and any(row[key] != rows[0][key] for key in ('active', 'prepared', 'attached')))):
            malformed += 1
        previous[row['route']] = row
        latest = row
        if len(rows) < 4:
            rows.append(row)
    if rows and len(rows) != 4:
        incomplete += 1
    health_result = parse_native_score_health(text)
    health = health_result['health']
    age = health[-1]['ms'] - latest['ms'] if latest and health else None
    # Independent five-second worker deadlines can put either report first.
    # Keep signed health-minus-binder age visible; bounded proximity establishes
    # diagnostic context only, never live freshness or complete event capture.
    health_ordered = all(after['ms'] >= before['ms'] and after['epoch'] >= before['epoch']
                         for before, after in zip(health, health[1:]))
    current_context = bool(latest and health and not health_result['health_malformed'] and
                           not health_result['health_dropped'] and not health[-1]['overflow'] and
                           health_ordered and
                           health[-1]['epoch'] == latest['epoch'] and health[-1]['active'] and
                           age is not None and abs(age) <= 6000)
    overflow = any(row['overflow'] for row in previous.values())
    complete = bool(len(rows) == 4 and not (malformed or dropped or incomplete or overflow) and
                    latest['active'] and latest['prepared'] and latest['attached'] and current_context)
    return dict(binder_snapshot=rows if complete else None, binder_latest_status=latest,
                binder_status='COMPLETE' if complete else 'UNVERIFIED' if received else 'MISSING',
                binder_malformed=malformed, binder_dropped=dropped, binder_incomplete=incomplete,
                binder_rows_seen=received, binder_overflow=overflow, binder_context_current=current_context,
                binder_health_age_ms=age, binder_health_abs_age_ms=abs(age) if age is not None else None,
                binder_evidence_gap=bool(malformed or dropped or incomplete or overflow or (received and not current_context)),
                binder_capture_completeness='UNVERIFIED', binder_freshness='UNVERIFIED', binder_owner='UNVERIFIED')


def parse_native_score_events(text):
    """Read-only expression payloads; serials are not deduplicated native identities."""
    samples, status, guards, dropped, status_dropped, guard_dropped, malformed = [], [], [], 0, 0, 0, 0
    for line in text.splitlines():
        prior_counts = (len(samples) + dropped, len(status) + status_dropped, len(guards) + guard_dropped)
        guard = re.search(r'ARCADE NATIVE SCORE GUARD: seq=([0-9]{1,20}) epoch=([0-9]{1,20}) delivered=([0-9]{1,20}) decision=([235]) authoritative=false\.$', line)
        if guard:
            seq, epoch, delivered, decision = map(int, guard.groups())
            if seq and epoch and max(seq, epoch, delivered) <= 0xffffffffffffffff:
                guards.append(dict(seq=seq, epoch=epoch, delivered=delivered, decision=decision, authoritative=False))
                if len(guards) > 128:
                    guards.pop(0)
                    guard_dropped += 1
        row = re.search(r'ARCADE NATIVE SCORE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) epoch=([0-9]{1,20}) route=([1-4]) score=([0-9.eE+-]{1,32}) reason=([0-8]) success=(true|false) owner_verified=false\.$', line)
        if row:
            seq, ms, epoch, route, score, reason, success = row.groups()
            seq, ms, epoch, route, reason = map(int, (seq, ms, epoch, route, reason))
            try:
                score = float(score)
            except ValueError:
                malformed += 1
                continue
            if (not seq or not epoch or max(seq, ms, epoch) > 0xffffffffffffffff or
                    not math.isfinite(score) or not 0 <= score <= 2147483647 or
                    (route == 1 and (score or reason or success != 'false')) or
                    (route in (2, 3) and success != 'false') or (route == 4 and reason)):
                malformed += 1
                continue
            samples.append(dict(seq=seq, ms=ms, epoch=epoch, route=route, score=score,
                                reason=reason, success=success == 'true', owner_verified=False))
            if len(samples) > 128:
                samples.pop(0)
                dropped += 1
        row = re.search(r'ARCADE NATIVE SCORE STATUS: epoch=([0-9]{1,20}) active=(true|false) bound=(true|false) dropped=([0-9]{1,20}) rejected=([0-9]{1,20}) contended=([0-9]{1,20}) stale=([0-9]{1,20}) owner_verified=false\.$', line)
        if row:
            epoch, active, bound, lost, rejected, contended, stale = row.groups()
            epoch, lost, rejected, contended, stale = map(int, (epoch, lost, rejected, contended, stale))
            if (not epoch or max(epoch, lost, rejected, contended, stale) > 0xffffffffffffffff or
                    (active == 'false' and bound == 'true')):
                malformed += 1
                continue
            status.append(dict(epoch=epoch, active=active == 'true', bound=bound == 'true',
                               dropped=lost, rejected=rejected, contended=contended, stale=stale,
                               owner_verified=False))
            if len(status) > 32:
                status.pop(0)
                status_dropped += 1
        if ('ARCADE NATIVE SCORE:' in line or 'ARCADE NATIVE SCORE STATUS:' in line or
                'ARCADE NATIVE SCORE GUARD:' in line) and prior_counts == (
                    len(samples) + dropped, len(status) + status_dropped, len(guards) + guard_dropped):
            malformed += 1
    return dict(samples=samples, status=status, guards=guards, dropped=dropped, status_dropped=status_dropped,
                guard_dropped=guard_dropped, malformed=malformed,
                native_event_identity='UNVERIFIED', **parse_native_score_details(text),
                **parse_native_score_health(text), **parse_native_score_schema(text), **parse_native_score_binder(text))


def parse_fixture_trajectory(text):
    """Opt-in relative motion diagnostics; never native mechanic/owner proof."""
    names = ('session seq ms sampled generation revision status dropped x_mm y_mm z_mm '
             'speed_mm_s heading_mdeg state').split()
    statuses = ('idle', 'armed', 'sample', 'complete', 'invalid', 'gap', 'timeout', 'cancelled', 'dropped')
    marker = 'ARCADE FIXTURE TRAJECTORY: '
    pattern = re.compile(re.escape(marker) + ' '.join(
        k + r'=(-?[0-9]{1,20})' for k in names) + r'\.$')
    result = dict(samples=[], status=None, malformed=0, dropped=0, sequence_missing=0,
                  capture_completeness='UNVERIFIED', owner_proof='UNVERIFIED')
    previous = None
    for line in text.splitlines():
        if 'ARCADE FIXTURE TRAJECTORY STATUS: ' in line:
            try:
                raw = line.split('ARCADE FIXTURE TRAJECTORY STATUS: ', 1)[1]
                if len(raw) > 512:
                    raise ValueError('size')
                def unique(pairs):
                    out = {}
                    for k, v in pairs:
                        if k in out:
                            raise ValueError('duplicate')
                        out[k] = v
                    return out
                row = json.loads(raw, object_pairs_hook=unique)
                if (not isinstance(row, dict) or set(row) != {'session', 'status', 'samples', 'dropped', 'duration_ms', 'sample_limit'} or
                        any(type(v) is not int or not 0 <= v <= 2**64-1 for v in row.values()) or
                        row['status'] > 8 or row['samples'] > 481 or row['duration_ms'] != 24000 or row['sample_limit'] != 481):
                    raise ValueError('status')
                row['status'] = statuses[row['status']]
                result['status'] = row
            except (ValueError, TypeError):
                result['malformed'] += 1
            continue
        if marker not in line:
            continue
        match = pattern.search(line)
        try:
            if not match:
                raise ValueError('row')
            row = dict(zip(names, map(int, match.groups())))
            if (any(not 0 <= row[k] <= 2**64-1 for k in names if k not in ('x_mm', 'y_mm', 'z_mm')) or
                    not row['session'] or not 1 <= row['seq'] <= 482 or not row['generation'] or not row['revision'] or
                    row['status'] not in range(2, 9) or abs(row['x_mm']) > 100000 or abs(row['y_mm']) > 20000 or
                    abs(row['z_mm']) > 100000 or row['speed_mm_s'] > 50000 or row['heading_mdeg'] > 360000 or row['state'] > 10000):
                raise ValueError('bounds')
            valid = row['status'] == 2
            if valid:
                if not row['sampled'] or not 0 <= row['ms'] - row['sampled'] <= 150 or row['dropped']:
                    raise ValueError('freshness')
            elif any(row[k] for k in ('sampled', 'x_mm', 'y_mm', 'z_mm', 'speed_mm_s', 'heading_mdeg', 'state')):
                raise ValueError('terminal values')
            if previous:
                if row['session'] < previous['session']:
                    raise ValueError('old session')
                if row['session'] == previous['session']:
                    if (row['seq'] <= previous['seq'] or previous['status'] != 2 or
                            row['generation'] != previous['generation'] or row['revision'] != previous['revision'] or
                            (row['ms'] < previous['ms'] and row['status'] != 5) or
                            (valid and row['sampled'] <= previous['sampled'])):
                        raise ValueError('reordered context')
                    result['sequence_missing'] += row['seq'] - previous['seq'] - 1
                else:
                    result['sequence_missing'] += row['seq'] - 1
            else:
                result['sequence_missing'] += row['seq'] - 1
            previous = dict(row)
            row['status'] = statuses[row['status']]
            row['valid'] = valid
            if not valid:
                for k in ('sampled', 'x_mm', 'y_mm', 'z_mm', 'speed_mm_s', 'heading_mdeg', 'state'):
                    row[k] = None
            result['samples'].append(row)
            if len(result['samples']) > 512:
                result['samples'].pop(0)
                result['dropped'] += 1
        except (ValueError, TypeError):
            result['malformed'] += 1
    result['evidence_gap'] = bool(result['malformed'] or result['dropped'] or result['sequence_missing'] or
        any(r['status'] in ('invalid', 'gap', 'timeout', 'cancelled', 'dropped') or r['dropped'] for r in result['samples']) or
        (result['status'] and (result['status']['dropped'] or result['status']['status'] in ('invalid', 'gap', 'timeout', 'cancelled', 'dropped'))))
    return result


def parse_fixture_geometry(text):
    """Latest bounded picking diagnostic, never contact or grind authority.

    A malformed newer declaration clears an older result. Status 2 can remain
    useful historical geometry with context_current=false; it is not a live
    fixture identity or a complete mesh bounding box.
    """
    result = None
    marker = 'ARCADE FIXTURE GEOMETRY: '
    base_keys = {'status', 'samples', 'budget', 'step', 'radius', 'contact_verified',
                 'session', 'started_ms', 'generation', 'revision', 'object', 'context_current'}
    rail_keys = {'rail_position', 'rail_rotation', 'rail_scale'}
    bounds_keys = {'sampled_low', 'sampled_high', 'height_min', 'height_max', 'edge_uncertainty'}
    def unique_object(pairs):
        row = {}
        for key, value in pairs:
            if key in row:
                raise ValueError('Duplicate geometry field')
            row[key] = value
        return row
    def invalid_constant(_):
        raise ValueError('Nonfinite geometry field')
    def number(value, low, high):
        return type(value) in (int, float) and math.isfinite(value) and low <= value <= high
    def vector(value, count, low, high):
        return (type(value) is list and len(value) == count and
                all(number(item, low, high) for item in value))
    newest_session = 0
    for line in text.splitlines():
        if 'ARCADE FIXTURE GEOMETRY:' not in line:
            continue
        result = None
        if marker not in line:
            continue
        payload = line.partition(marker)[2]
        if not 2 <= len(payload) <= 4096:
            continue
        try:
            row = json.loads(payload, object_pairs_hook=unique_object, parse_constant=invalid_constant)
            if type(row) is not dict:
                continue
            integers = ('status', 'samples', 'budget', 'session', 'started_ms', 'generation', 'revision', 'object')
            if any(type(row.get(key)) is not int or not 0 <= row[key] <= 0xffffffffffffffff for key in integers):
                continue
            status, samples, session = row['status'], row['samples'], row['session']
            if session > 0xffffff or session < newest_session:
                continue
            newest_session = session
            if status > 8 or samples > 129 or row['budget'] != 129:
                continue
            if (not number(row.get('step'), .25, .25) or not number(row.get('radius'), 8, 8) or
                    row.get('contact_verified') is not False or type(row.get('context_current')) is not bool):
                continue
            expected = base_keys | (rail_keys if session else set()) | (bounds_keys if status == 2 else set())
            if set(row) != expected or (row['context_current'] and status != 2):
                continue
            if status == 0:
                if samples or session or row['started_ms'] or row['generation'] or row['revision'] or row['object']:
                    continue
            elif not session or not row['started_ms'] or not row['generation'] or not row['object']:
                continue
            if ((status in (2, 6, 7) and samples != 129) or
                    (status in (1, 4, 5) and samples >= 129) or (status == 8 and samples != 0)):
                continue
            if session:
                if (not vector(row['rail_position'], 3, -100000, 100000) or
                        not vector(row['rail_rotation'], 4, -1, 1) or
                        not .99 <= sum(value * value for value in row['rail_rotation']) <= 1.01 or
                        not number(row['rail_scale'], .01, 100)):
                    continue
            if status == 2:
                if (not vector(row['sampled_low'], 2, -7.75, 0) or
                        not vector(row['sampled_high'], 2, 0, 7.75) or
                        any(value * 4 != int(value * 4) for value in row['sampled_low'] + row['sampled_high']) or
                        not number(row['edge_uncertainty'], .25, .25) or
                        not number(row['height_min'], -0.10000000149011612, 2) or
                        not number(row['height_max'], row['height_min'], 2)):
                    continue
            result = row
        except (ValueError, TypeError, OverflowError, RecursionError):
            continue
    return result


def parse_fixture_status(text):
    result = None
    booleans = ('configured', 'available', 'busy', 'failed', 'editor_open', 'asset_available', 'present', 'skater', 'reset_ready')
    for line in text.splitlines():
        if 'ARCADE FIXTURE: ' not in line:
            continue
        # A malformed later response cannot leave an earlier ready/menu-closed
        # snapshot masquerading as the latest command result.
        result = None
        match = re.search(r'ARCADE FIXTURE: (\{.{1,4096}\})$', line)
        if not match:
            continue
        try:
            row = json.loads(match[1])
            if isinstance(row, dict) and row.get('configured') is False:
                row.setdefault('reset_ready', False)
                row.setdefault('start', [0, 0, 0])
                row.setdefault('rail_position', [0, 0, 0])
                row.setdefault('rail_rotation', [0, 0, 0, 1])
                row.setdefault('rail_scale', 1)
            if not isinstance(row, dict) or any(type(row.get(k)) is not bool for k in booleans):
                continue
            numeric = ('heading', 'speed', 'state', 'generation', 'revision', 'rail_scale')
            if any(type(row.get(k)) not in (int, float) or not math.isfinite(row[k]) or abs(row[k]) > 1e12 for k in numeric):
                continue
            if any(type(row[k]) is not int or row[k] < 0 for k in ('state', 'generation', 'revision')) or row['speed'] < 0:
                continue
            arrays = {'position': 3, 'start': 3, 'rail_position': 3, 'rail_rotation': 4}
            if any(not isinstance(row.get(k), list) or len(row[k]) != length or
                   any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 100000 for v in row[k])
                   for k, length in arrays.items()):
                continue
            if not .01 <= row['rail_scale'] <= 100 or not .99 <= sum(v*v for v in row['rail_rotation']) <= 1.01:
                continue
            result = {k: row[k] for k in (*booleans, *numeric, *arrays)}
            if type(row.get('arcade')) is bool:
                result['arcade'] = row['arcade']
            # Historical debug menu transitions are not a current snapshot.
            # Only these paired fields from the fresh fixture command qualify.
            if 'native_ui_available' in row or 'native_in_menu' in row:
                available, menu = row.get('native_ui_available'), row.get('native_in_menu')
                state = 'invalid'
                paired = 'native_ui_available' in row and 'native_in_menu' in row
                if paired and available is True and type(menu) is bool:
                    state = 'open' if menu else 'closed'
                elif paired and available is False and menu is None:
                    state = 'unavailable'
                result['native_menu'] = state
                result['native_ui_available'] = available if state != 'invalid' else None
                result['native_in_menu'] = menu if state in ('open', 'closed') else None
        except (ValueError, TypeError):
            continue
    return result


def parse_grind_observations(text):
    """Fixed numeric records only. Raw enums deliberately have no named mapping."""
    result = dict(samples=[], pad=[], status=[], discovery=[], dropped=0, native_identity='UNKNOWN')
    patterns = {
        'samples': (r'ARCADE GRIND STATE: seq=(\d{1,20}) ms=(\d{1,20}) generation=(\d{1,20}) valid=(true|false) state=(\d{1,10}) edge=([0-3]) duration=(\d{1,20}) grind_valid=(true|false) grind=(\d{1,10}) skate_valid=(true|false) skate=(\d{1,10}) speed_valid=(true|false) speed_mm_s=(\d{1,7}) arcade=(true|false) dropped=(\d{1,20})\.$',
                    'seq ms generation valid state edge duration grind_valid grind skate_valid skate speed_valid speed_mm_s arcade dropped'),
        'pad': (r'ARCADE GRIND PAD: seq=(\d{1,20}) ms=(\d{1,20}) generation=(\d{1,20}) user=([0-3]) y=(true|false) x=(-?\d{1,5}) ry=(-?\d{1,5}) arcade=(true|false) dropped=(\d{1,20})\.$',
                'seq ms generation user requested_y x y arcade dropped'),
        'status': (r'ARCADE GRIND STATUS: generation=(\d{1,20}) active=(true|false) dropped=(\d{1,20}) rejected=(\d{1,20})\.$',
                   'generation active dropped rejected'),
        'discovery': (r'ARCADE GRIND DISCOVERY: generation=(\d{1,20}) bound=([0-3]) pages=(\d{1,20}) passes=(\d{1,20})\.$',
                      'generation bound pages passes')}
    for line in text.splitlines():
        for kind, (pattern, fields) in patterns.items():
            match = re.search(pattern, line)
            if not match:
                continue
            row = {key: value == 'true' if value in ('true', 'false') else int(value)
                   for key, value in zip(fields.split(), match.groups())}
            if (not row['generation'] or any(type(v) is int and not -32768 <= v <= 0xffffffffffffffff for v in row.values()) or
                    any(row.get(k, 0) > 0xffffffff for k in ('state', 'grind', 'skate')) or
                    any(not -32768 <= row.get(k, 0) <= 32767 for k in ('x', 'y')) or row.get('speed_mm_s', 0) > 1000000):
                continue
            result[kind].append(row)
            # Pad samples arrive at 20 Hz: cover a 12s trial plus 10s recovery.
            if len(result[kind]) > (512 if kind == 'pad' else 256):
                result[kind].pop(0)
                result['dropped'] += 1
    return result


def parse_authored_catalog(text):
    entries, status, total = {}, None, None
    labels = {}
    arc_data = {}
    diagnostic = None
    conflicting = False
    for line in text.splitlines():
        arc = re.search(r'ARCADE AUTHORED ARC: index=([0-9]{1,3}) arc=([0-9]{1,2}) start=(-?[0-9]{1,7}) end=(-?[0-9]{1,7}) min=([0-9]{1,4}) max=([0-9]{1,4}) wait=(-?[0-9]{1,5})\.$', line)
        if arc:
            index, ordinal, start, end, minimum, maximum, wait = map(int, arc.groups())
            if (index < 256 and ordinal < 32 and abs(start) <= 3600000 and abs(end) <= 3600000
                    and 0 <= minimum <= 1000 and minimum <= maximum <= 2000 and -1000 <= wait <= 10000):
                value = dict(start_mdeg=start, end_mdeg=end, min_milli=minimum, max_milli=maximum, wait_ms=wait)
                key = (index, ordinal)
                if key in arc_data and arc_data[key] != value:
                    conflicting = True
                arc_data[key] = value
        label = re.search(r'ARCADE AUTHORED LABEL: index=([0-9]{1,3}) tokens=([0-9]{1,4})\.$', line)
        if label and int(label[1]) < 256 and int(label[2]) <= 1023:
            index, tokens = map(int, label.groups())
            if index in labels and labels[index] != tokens:
                conflicting = True
            labels[index] = tokens
        state = re.search(r'ARCADE AUTHORED STATUS: state=([0-3])\.$', line)
        if state:
            status = int(state[1])
        failure = re.search(r'ARCADE AUTHORED DIAGNOSTIC: stage=([0-7]) field=([0-9]{1,2}) error=([0-4]) rejected=([0-9]{1,5})\.$', line)
        if failure and int(failure[2]) <= 14 and int(failure[4]) <= 65535:
            diagnostic = dict(zip(('stage', 'field', 'error', 'rejected'), map(int, failure.groups())))
        row = re.search(r'ARCADE AUTHORED GESTURE: index=([0-9]{1,3}) total=([0-9]{1,3}) id=([0-9]{1,2}) nollie=(true|false) context=([0-2]) arcs=([0-9]{1,2})\.$', line)
        if not row:
            continue
        index, count, native_id, arcs = int(row[1]), int(row[2]), int(row[3]), int(row[6])
        if not (0 <= index < count <= 256 and 1 <= native_id <= 63 and 1 <= arcs <= 32):
            continue
        entry = {'index': index, 'id': native_id, 'nollie': row[4] == 'true', 'context': int(row[5]), 'arcs': arcs}
        if (total is not None and total != count) or (index in entries and entries[index] != entry):
            conflicting = True
        total = count
        entries[index] = entry
    for index, entry in entries.items():
        if index in labels:
            entry['source_tokens'] = labels[index]
        if any(key[0] == index for key in arc_data):
            entry['arc_data'] = [dict(arc=ordinal, **arc_data[index, ordinal])
                                for ordinal in range(entry['arcs']) if (index, ordinal) in arc_data]
            entry['arcs_complete'] = len(entry['arc_data']) == entry['arcs'] and not conflicting
    return {'status': status, 'entries': [entries[i] for i in sorted(entries)], 'diagnostic': diagnostic,
            'partial': bool(diagnostic and diagnostic['rejected']),
            'complete': status == 2 and total is not None and not conflicting and
                        not (diagnostic and diagnostic['rejected']) and set(entries) == set(range(total))}


def native_trace(request, **kwargs):
    verified = evidence(request, **kwargs)
    if not verified["fresh_log"]:
        raise ValueError("Native trace requires a fresh log from the active installed DLL")
    status = snapshot(request, **kwargs)
    receipt = read_json(Path(status["game"]) / RECEIPT)
    path = Path(receipt["log_path"])
    if path.name != "ReSkate.log" or any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError("Unsafe native trace log")
    if path.stat().st_size > 16 * 1024 * 1024:
        raise ValueError("Native trace log too large")
    text = path.read_text(errors="replace")
    start = text.rfind("Arcade Mode ready")
    captures = parse_native_captures(text[start:] if start >= 0 else "")
    return {"schema": 1, "kind": "native_request_trace",
            **{key: verified[key] for key in ("source_commit", "installed_dll_sha256", "installation_id", "timestamp")},
            "captures": captures, "combo_observations": parse_combo_samples(text[start:] if start >= 0 else ""),
            "score_observations": parse_score_samples(text[start:] if start >= 0 else ""),
            "push_observations": parse_push_samples(text[start:] if start >= 0 else ""),
            "pad_observations": parse_pad_samples(text[start:] if start >= 0 else ""),
            "flip_observations": parse_flip_observations(text[start:] if start >= 0 else ""),
            "stick_observations": parse_stick_samples(text[start:] if start >= 0 else ""),
            "playback_observations": parse_playback_samples(text[start:] if start >= 0 else ""),
            "action_observations": parse_action_samples(text[start:] if start >= 0 else ""),
            "score_edge_observations": parse_score_edges(text[start:] if start >= 0 else ""),
            "score_label_observations": parse_score_presence(text[start:] if start >= 0 else ""),
            "score_number_observations": parse_score_numbers(text[start:] if start >= 0 else ""),
            "native_score_observations": parse_native_score_events(text[start:] if start >= 0 else ""),
            "grind_observations": parse_grind_observations(text[start:] if start >= 0 else ""),
            "fixture": parse_fixture_status(text[start:] if start >= 0 else ""),
            "fixture_geometry": parse_fixture_geometry(text[start:] if start >= 0 else ""),
            "fixture_trajectory": parse_fixture_trajectory(text[start:] if start >= 0 else ""),
            "authored_catalog": parse_authored_catalog(text[start:] if start >= 0 else "")}
