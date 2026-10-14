import re
MAX_PLAN = 65536
MAX_TRIALS = 24
AXIS_CODES = {"lx": 0, "ly": 1, "lt": 2, "rx": 3, "ry": 4, "rt": 5, "dx": 16, "dy": 17}
BUTTON_CODES = {"A": 0x130, "B": 0x131, "X": 0x133, "Y": 0x134, "LB": 0x136, "RB": 0x137}
HIL_ROW = re.compile(r"ARCADE HIL STATE: seq=([0-9]{1,20}) valid=(true|false) state=([0-9]{1,10}) mode=([0-9]{1,10})\.")
BUTTON_ROW = re.compile(r"ARCADE MODE DEV: ([ABXY]) press #([0-9]{1,20}) seen on ReSkate client tick\.")


class HardStop(RuntimeError):
    pass


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError("Integer outside bounded range")
    return value


def keys(value, required, optional=()):
    if not isinstance(value, dict) or set(value) - set(required) - set(optional) or set(required) - set(value):
        raise ValueError("Unknown or missing data fields")


def validate_plan(plan):
    keys(plan, ("schema", "source_commit", "dll_sha256", "stock_sha256", "candidates"))
    if type(plan["schema"]) is not int or plan["schema"] != 1:
        raise ValueError("Unsupported schema")
    for field, length in (("source_commit", 40), ("dll_sha256", 64), ("stock_sha256", 64)):
        if not isinstance(plan[field], str) or not re.fullmatch("[0-9a-f]{%d}" % length, plan[field]):
            raise ValueError("Invalid source or hash")
    cases = plan["candidates"]
    if not isinstance(cases, list) or not 1 <= len(cases) <= MAX_TRIALS:
        raise ValueError("Batch must contain 1..24 candidates")
    names, trials = set(), 0
    for case in cases:
        keys(case, ("name", "family", "phases", "expected", "timeout_ms", "reset_ms"), ("repeat", "capture_frames"))
        for field in ("name", "family"):
            if not isinstance(case[field], str) or not re.fullmatch(r"[a-z][a-z0-9-]{0,47}", case[field]):
                raise ValueError("Invalid candidate label")
        if case["name"] in names:
            raise ValueError("Duplicate candidate")
        names.add(case["name"])
        trials += integer(case.get("repeat", 1), 1, 20)
        if integer(case.get("capture_frames", 4), 1, 4) not in (1, 4):
            raise ValueError("Capture count must be one or four")
        timeout = integer(case["timeout_ms"], 3000, 12000)
        integer(case["reset_ms"], 2000, 10000)
        keys(case["expected"], ("states",), ("trick",))
        states = case["expected"]["states"]
        if not isinstance(states, list) or len(states) > 12:
            raise ValueError("Invalid expected states")
        for state in states:
            integer(state, 0, 10000)
        if "trick" in case["expected"] and (not isinstance(case["expected"]["trick"], str) or
                not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,47}", case["expected"]["trick"])):
            raise ValueError("Invalid expected trick")
        phases = case["phases"]
        if not isinstance(phases, list) or not 1 <= len(phases) <= 64:
            raise ValueError("Invalid phases")
        previous = -1
        for phase in phases:
            keys(phase, ("ms",), tuple(AXIS_CODES) + ("buttons",))
            ms = integer(phase["ms"], 0, timeout - 1000)
            if ms <= previous:
                raise ValueError("Phases must increase strictly")
            previous = ms
            for axis in AXIS_CODES:
                limit = (0, 255) if axis in ("lt", "rt") else (-1, 1) if axis in ("dx", "dy") else (-32768, 32767)
                integer(phase.get(axis, 0), *limit)
            buttons = phase.get("buttons", [])
            if (not isinstance(buttons, list) or any(not isinstance(b, str) or b not in BUTTON_CODES for b in buttons)
                    or len(set(buttons)) != len(buttons)):
                raise ValueError("Invalid buttons")
    if trials > MAX_TRIALS:
        raise ValueError("At most 24 trials per batch")
    return plan
