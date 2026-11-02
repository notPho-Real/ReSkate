"""Offline pinned402->406->403 control/completion evidence; no hardware access.

Usage: python analyze_grind_slide_return.py /absolute/path/to/reskate-arcade-lab
This does not verify production animation, HUD, balance or combo acceptance.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def receipt(root, version, digest):
    path = root / f"grind-native-slide-third-c048914-{version}" / "results/two-leg-lateral.json.gz"
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == digest, path
    envelope = json.loads(gzip.decompress(blob))
    assert envelope["returncode"] == 0
    return json.loads(envelope["stdout"])


def gate(data):
    return next(json.loads(line.split(" ", 1)[1]) for line in data["gate_log"]
                if line.startswith("GRIND_GATE_RESULT "))


def trial(data):
    return next(t for t in data["trials"] if t["recipe"].startswith("native-long-rail-"))["delivery_timing"]


def analyze(root):
    rejected = receipt(root, "v1", "aca75b3a2372e48d7936d180ee3e08a4c1ff22043eff154aaaca80adad9212d0")
    assert rejected["error"] == "AssertionError: Native rail exit before original402 controls"
    assert len(rejected["native_samples"]) == 296
    assert gate(rejected)["legs"] == 2 and not gate(rejected)["third_ready"]
    assert rejected["partial_run"]["neutralized"]
    assert rejected["partial_run"]["delivery_timing"]["interrupted_phase"] == 7
    assert all(s["scheduled_ms"] < 2700 for s in rejected["partial_run"]["delivery_timing"]["samples"])
    assert rejected["gate_returncode"] == 0 and rejected["gate_restored"]
    assert rejected["original_fixture_absence_restored"] and rejected["after"]["revision"] == 434
    # The first rejected selector value was not captured. Do not infer a
    # completed-frame rail exit or a harmless transient from adjacent readers.
    assert "third_ready_reads" not in rejected

    data = receipt(root, "v2", "4dd8538d0c8b4feab2a56a16f3ce1c18f712d739d32e0ffb458cc0dd75bd1c2d")
    assert "error" not in data and data["gate_returncode"] == 0 and data["gate_restored"]
    assert data["original_fixture_absence_restored"] and data["after"]["revision"] == 458
    assert data["after"]["arcade"] and not data["after"]["native_in_menu"] and not data["after"]["editor_open"]
    rows = data["native_samples"]
    assert len(rows) == 614
    assert all(abs(r["physics_dt"] - 1 / 30) < 1e-7 and abs(r["cached_dt"] - 1 / 30) < 1e-7 for r in rows)
    origin = rows[0]["ns"]
    def ms(row):
        return (row["ns"] - origin) / 1e6
    result = gate(data)
    assert result["legs"] == 3 and result["reason"] == "third_native_identity_observed"
    assert [(c["source"], c["target"]) for c in result["completed"]] == [(401, 403), (403, 402), (402, 403)]
    cleanup = next(a for a in result["actor_events"] if a.get("leg") == 3 and a["boundary"] == "cleanup")
    assert cleanup["source"] == 402 and cleanup["target"] == 403 and cleanup["timing"][-1] == 1.0
    assert abs(cleanup["timing"][6] - .5) < 1e-7
    timing = trial(data)
    assert not timing["condition_failed"] and [c["phase"] for c in timing["conditions"]] == [4, 6, 7]
    assert all(c["status"] == "READY" for c in timing["conditions"])
    assert timing["conditions"][-1]["observation"]["gate_waiting"]
    assert all(r["state"] == r["state_again"] == 402 and r["named"] == 31 and
               r["physical"] == r["published"] == 1 and r["feature"] == 0 for r in data["third_ready_reads"])
    writes = {s["scheduled_ms"]: s["write_ms"] for s in timing["samples"]}
    assert abs(writes[2900] - writes[2700] - 200) < 1
    assert abs(writes[3500] - writes[2900] - 600) < 1
    segment = [4045859192871, 0]
    stable = [(r, b) for r in rows if (b := r.get("admission_sample")) and b["stable_reads"] and
              b["segment_identity"] == segment and b["physical_contact"] == b["published_contact"] == 1 and
              b["missing_segment"] == 0]
    admitted = [(r, b) for r, b in stable if b["state"] == 402 and b["named"] == 31 and
                b["actor_active"] == b["published_admission"] == 1]
    assert len(admitted) == 2
    assert all(b["requested_orientation_raw"] == [0.19176197052001953, -0.34948572516441345, 0.0]
               for _, b in admitted)
    destination = [(r, b) for r, b in stable if b["state"] == 403 and b["named"] == 24 and
                   r["ns"] >= result["completed"][-1]["ns"]]
    assert len(destination) == 18 and all(r["allowed"] == 1 for r, _ in destination)
    assert all(r["active_segment"]["endpoints"] == [[144.9501953125, 0.5007421970367432, -44.1923828125],
                                                     [144.9501953125, 0.5007421970367432, -56.1923828125]]
               for r, _ in destination)
    assert all(b["actor"] == 4137964656 for _, b in destination)
    neutral_return = [(r, b) for r, b in destination if b["actor_active"] == b["published_admission"] == 1]
    assert len(neutral_return) == 13
    assert all(b["requested_orientation_raw"][1:] == [0.0, 0.0] for _, b in neutral_return)
    mixed = [ms(r) for r, _ in stable if r["allowed"] == 0]
    assert mixed == [3334.823785]
    return {
        "original_legs": result["completed"],
        "third_original_completion_elapsed_seconds": cleanup["timing"][6],
        "writer_ms": {"down": writes[2700], "diagonal": writes[2900], "neutral": writes[3500]},
        "admission_reader_ms": [ms(r) for r, _ in admitted],
        "independent_5_0": {"stable_same_segment_samples": len(destination),
                             "reader_ms": [ms(destination[0][0]), ms(destination[-1][0])]},
        "later_neutral_5_0_admission": {"stable_samples": len(neutral_return),
                                        "reader_ms": [ms(neutral_return[0][0]), ms(neutral_return[-1][0])]},
        "mixed_time_contact_samples_ms": mixed,
        "limits": ["ArcadeOFF native physics/control evidence; not production animation/HUD/balance/combo acceptance.",
                   "Only402/fine31 to403/fine24 qualified; no fourth native leg or arbitrary slide source.",
                   "v1 failed before third controls; its rejected selector value remains unknown.",
                   "Sequential outer and bracket reads do not prove uninterrupted completed-frame contact."],
    }


if __name__ == "__main__":
    assert len(sys.argv) == 2, __doc__
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2))
