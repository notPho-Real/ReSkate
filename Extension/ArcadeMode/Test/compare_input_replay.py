"""Compare identical controller traces; permit only documented diagnostic changes."""
import argparse
import collections
import csv
import io
import json
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline")
    parser.add_argument("replacement")
    args = parser.parse_args()
    results = []
    for executable in (args.baseline, args.replacement):
        run = subprocess.run([executable], capture_output=True, text=True, check=True)
        if run.stderr:
            raise AssertionError(run.stderr)
        results.append(list(csv.reader(io.StringIO(run.stdout))))
    baseline, replacement = results
    assert len(baseline) == len(replacement), "sample count changed"
    accepted = collections.Counter()
    identical = 0
    for before, after in zip(baseline, replacement):
        assert len(before) == len(after) == 21, "controller evidence schema changed"
        assert before[:3] == after[:3], "trace/sample identity changed"
        if before == after:
            identical += 1
            continue
        changes = {i: (a, b) for i, (a, b) in enumerate(zip(before, after)) if a != b}
        scenario = before[0].split("-")
        assert scenario[0] == "action", (before, after)
        case = int(scenario[-1])
        if case == 15:
            if changes == {11: ("0", "1")}:
                accepted["explicit_unsupported_balance_branch"] += 1
            else:
                assert changes == {15: ("1", "0"), 18: ("2", "0")}, changes
                accepted["balance_branch_without_delayed_grind_diagnostic"] += 1
        elif case == 32:
            # Ground B remains native braking. Remove the old rejected grab
            # diagnostic; every delivered controller field is identical.
            assert changes == {15: ("1", "0"), 17: ("54", "0"), 18: ("1", "0"), 19: ("3", "1")}, changes
            accepted["ground_brake_without_grab_diagnostic"] += 1
        elif case == 46:
            # An unverified nollie grab was admitted then immediately cancelled
            # by the old path. Report unavailable without claiming admission.
            assert changes == {19: ("0", "4")}, changes
            accepted["unverified_context_unavailable"] += 1
        else:
            raise AssertionError((before, after))
    assert dict(accepted) == {"explicit_unsupported_balance_branch": 2,
                              "balance_branch_without_delayed_grind_diagnostic": 2,
                              "ground_brake_without_grab_diagnostic": 2,
                              "unverified_context_unavailable": 2}, accepted
    print(json.dumps({"samples": len(baseline), "identical": identical,
                      "intentional_diagnostic_differences": dict(accepted),
                      "unexpected_differences": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
