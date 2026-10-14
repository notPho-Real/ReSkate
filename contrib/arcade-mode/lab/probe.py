"""Read-only Deck inspection. Sent in memory over SSH; never installed on Deck.

No installer import, filesystem writes, process signals, or shell execution.
The same functions run against temporary fake Decks in tests.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import time

SETTINGS = "ReSkateLauncher.settings.json"
BACKUP = "ReSkate.dll.arcade-stock.bak"
RECEIPT = ".arcade-mode-install.json"


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
    games = [Path(request["game"])] if request.get("game") else sorted(candidates)
    proc = processes() if process_info is None else process_info
    result = {"ssh_connected": True, "user": __import__("pwd").getpwuid(os.getuid()).pw_name,
              "host": socket.gethostname(), "steam_libraries": [str(p) for p in libs],
              "games": [str(p) for p in games], "processes": proc, "pass": False}
    if len(games) != 1:
        result["reason"] = "Select exactly one game with --game"
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
    samples, events, balance, dropped = [], [], [], 0
    for line in text.splitlines():
        event = re.search(r'ARCADE COMBO EVENT: ms=([0-9]{1,20}) pose=([0-6]) active=(true|false) failed=(true|false) fatigue=([0-9.eE+-]{1,24}) grind=([0-9.eE+-]{1,24}) manual=([0-9.eE+-]{1,24}) native_failure_verified=false\.$', line)
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
    return {"samples": samples, "events": events, "balance": balance, "dropped": dropped, "native_event_identity": "UNVERIFIED"}


def parse_score_samples(text):
    fields, samples, nodes, values = [], [], [], []
    labels, labels_dropped = [], 0
    for line in text.splitlines():
        label = re.search(r'ARCADE SCORE LABEL: ms=([0-9]{1,20}) path=([0-9a-f]{8}(?:\.[0-9a-f]{8}){0,3}) id=([0-9]{1,2})\.$', line)
        if label and int(label[1]) <= 0xffffffffffffffff and 1 <= int(label[3]) <= 20:
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
                if len(sources) > 128:
                    sources.pop(0)
                    dropped = min(0xffffffffffffffff, dropped + 1)
        row = re.search(r'ARCADE PAD SAMPLE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) user=([0-3]) buttons=([0-9]{1,5}) lt=([0-9]{1,3}) rt=([0-9]{1,3}) output=([0-9]{1,5}) phase=([0-3])\.$', line)
        if not row:
            continue
        values = [int(v) for v in row.groups()]
        if max(values[:2]) > 0xffffffffffffffff or max(values[3], values[6]) > 65535 or max(values[4:6]) > 255:
            continue
        samples.append(dict(zip(('seq', 'ms', 'user', 'buttons', 'lt', 'rt', 'output', 'phase'), values)))
        if len(samples) > 128:
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
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE FLIP OBSERVATION: ms=([0-9]{1,20}) valid=(true|false) id=(-?[0-9]{1,2})\.$', line)
        if not row or int(row[1]) > 0xffffffffffffffff or not -1 <= int(row[3]) <= 63:
            continue
        samples.append({'ms': int(row[1]), 'valid': row[2] == 'true', 'id': int(row[3])})
        if len(samples) > 128:
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
        if len(samples) > 128:
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
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_action_samples(text):
    """Numeric input-dispatch evidence only; seq joins the independent stick row."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE ACTION SAMPLE: seq=([0-9]{1,20}) ms=([0-9]{1,20}) tick=([0-9]{1,20}) user=([0-3]) mode=([0-4]) requested=(true|false) kind=([0-4]) trick=([0-9]{1,2}) result=([0-5]) owned=(true|false) lx=(-?[0-9]{1,5}) ly=(-?[0-9]{1,5}) lt=([0-9]{1,3}) rt=([0-9]{1,3}) packet=([0-9]{1,10})\.$', line)
        if not row:
            continue
        names = ('seq', 'ms', 'tick', 'user', 'mode', 'requested', 'kind', 'trick',
                 'result', 'owned', 'lx', 'ly', 'lt', 'rt', 'packet')
        sample = {key: value == 'true' if key in ('requested', 'owned') else int(value)
                  for key, value in zip(names, row.groups())}
        # ArcadeTrick::darkslide is 53. result is DispatchResult, never native success.
        if (max(sample['seq'], sample['ms'], sample['tick']) > 0xffffffffffffffff or
                sample['trick'] > 53 or sample['packet'] > 0xffffffff or
                not (-32768 <= sample['lx'] <= 32767 and -32768 <= sample['ly'] <= 32767) or
                max(sample['lt'], sample['rt']) > 255):
            continue
        samples.append(sample)
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


def parse_score_edges(text):
    """Numeric label appearances; slot roles and native action identity remain unverified."""
    samples, dropped = [], 0
    for line in text.splitlines():
        row = re.search(r'ARCADE SCORE EDGE: seq=([0-9]{1,20}) observed=([0-9]{1,20}) sampled=([0-9]{1,20}) labels=([0-9]{1,7}) uncertainty=([0-5])\.$', line)
        if not row:
            continue
        seq, observed, sampled, labels, uncertainty = map(int, row.groups())
        if max(seq, observed, sampled) > 0xffffffffffffffff or labels > 1048575:
            continue
        samples.append(dict(seq=seq, observed=observed, sampled=sampled, labels=labels, uncertainty=uncertainty))
        if len(samples) > 128:
            samples.pop(0)
            dropped += 1
    return {'samples': samples, 'dropped': dropped}


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
            "score_number_observations": parse_score_numbers(text[start:] if start >= 0 else ""),
            "authored_catalog": parse_authored_catalog(text[start:] if start >= 0 else "")}
