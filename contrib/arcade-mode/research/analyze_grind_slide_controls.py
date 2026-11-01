"""Offline analysis of pinned native402 control receipts; no hardware access.

Usage: python analyze_grind_slide_controls.py /absolute/path/to/reskate-arcade-lab
Admission is evidence for a future transition, not completion of that transition.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def receipt(root, version, digest):
    path = root / f"grind-native-slide-heading-c048914-{version}" / "results/two-leg-lateral.json.gz"
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == digest, path
    envelope = json.loads(gzip.decompress(blob))
    assert envelope["returncode"] == 0
    return json.loads(envelope["stdout"])


def analyze(root):
    rejected = receipt(root, "v1", "b83c2c353cbc0666a62e1dc4d6faff086acc835edfd78480bdb284e9ebce6d6d")
    assert rejected["error"] == "ValueError: Invalid bounded phase conditions"
    assert len(rejected["native_samples"]) == 3
    assert all(r["state"] == 100 for r in rejected["native_samples"])
    assert rejected["original_fixture_absence_restored"] and rejected["after"]["revision"] == 366
    ui = receipt(root, "v2", "bbdc899cae3c33f96397bcf803614ff14ce29af5067cbb92c2748b8a1ae3a9ea")
    assert ui["error"] == "AssertionError: Native console did not echo the exact requested command"
    assert not ui["native_samples"] and ui["after"]["revision"] == 366
    geometry = receipt(root, "v3", "00533c0a9b83be72e0ce3bab2456a559ee18295d9345b6ea476001743661e767")
    assert len(geometry["native_samples"]) == 270
    assert not any(400 <= r["state"] <= 406 for r in geometry["native_samples"])
    failed_trial = next(t for t in geometry["trials"] if t["recipe"].startswith("native-long-rail-"))
    assert failed_trial["delivery_timing"]["condition_failed"]
    assert [c["phase"] for c in failed_trial["delivery_timing"]["conditions"]] == [4]
    assert geometry["original_fixture_absence_restored"] and geometry["after"]["revision"] == 386

    data = receipt(root, "v4", "93b8d400a057045c7beb30b22806fe439f00e94a2b01149f21c6859e92dff379")
    assert "error" not in data and data["gate_returncode"] == 0 and data["gate_restored"]
    assert data["original_fixture_absence_restored"] and data["after"]["revision"] == 410
    assert data["after"]["arcade"] and not data["after"]["native_in_menu"] and not data["after"]["editor_open"]
    rows = data["native_samples"]
    assert len(rows) == 616
    assert all(abs(r["physics_dt"] - 1 / 30) < 1e-7 and abs(r["cached_dt"] - 1 / 30) < 1e-7 for r in rows)
    origin = rows[0]["ns"]
    def milliseconds(row):
        return (row["ns"] - origin) / 1e6
    gate = next(json.loads(line.split(" ", 1)[1]) for line in data["gate_log"] if line.startswith("GRIND_GATE_RESULT "))
    assert gate["reason"] == "second_native_bluntslide_observed" and gate["legs"] == 2
    assert [(c["source"], c["target"]) for c in gate["completed"]] == [(401, 403), (403, 402)]
    trial = next(t for t in data["trials"] if t["recipe"].startswith("native-long-rail-"))
    timing = trial["delivery_timing"]
    assert not timing["condition_failed"]
    assert [c["phase"] for c in timing["conditions"]] == [4, 6, 7]
    assert all(c["status"] == "READY" for c in timing["conditions"])
    assert timing["conditions"][-1]["observation"]["detached"]
    assert timing["conditions"][-1]["observation"]["original_completed"]
    writes = {s["scheduled_ms"]: s["write_ms"] for s in timing["samples"]}
    assert abs(writes[2900] - writes[2700] - 200) < 1
    assert abs(writes[3500] - writes[2900] - 600) < 1

    identity = [3753801416743, 0]
    stable = [(r, b) for r in rows if (b := r.get("admission_sample"))
              and b["stable_reads"] and b["segment_identity"] == identity
              and b["physical_contact"] == b["published_contact"] == 1 and b["missing_segment"] == 0]
    admitted = [(r, b) for r, b in stable if b["state"] == 402 and b["named"] == 31
                and b["published_admission"] == 1 and b["actor_active"] == 1]
    assert len(admitted) == 47
    assert all(b["actor"] == 4138022496 and b["requested_orientation_raw"][0] == 0.1917705535888672 for _, b in admitted)
    assert all(r["active_segment"]["endpoints"] == [[144.7177734375, 0.5007421970367432, -44.58984375],
                                                     [144.7177734375, 0.5007421970367432, -56.58984375]] for r, _ in admitted)
    salad = [(r, b) for r, b in stable if b["state"] == 403 and b["named"] == 25]
    assert len(salad) == 18 and not any(b["published_admission"] for _, b in salad)
    third_control_ready = timing["conditions"][-1]["observation"]["ns"]
    assert not any(r["state"] == 406 for r in rows if r["ns"] >= third_control_ready)
    mixed = [(r, b) for r, b in stable if r["allowed"] == 0 and 3469 < milliseconds(r) < 4730]
    assert len(mixed) == 5
    return {
        "native402_open_admission": {
            "stable_same_segment_contact_samples": len(admitted),
            "reader_time_ms": [milliseconds(admitted[0][0]), milliseconds(admitted[-1][0])],
            "requested_heading": admitted[0][1]["requested_orientation_raw"][0],
            "requested_pitch_range": [min(b["requested_orientation_raw"][1] for _, b in admitted),
                                      max(b["requested_orientation_raw"][1] for _, b in admitted)],
        },
        "writer_time_ms": {"down": writes[2700], "diagonal": writes[2900], "neutral": writes[3500]},
        "later_same_segment_salad": {
            "stable_samples": len(salad),
            "reader_time_ms": [milliseconds(salad[0][0]), milliseconds(salad[-1][0])],
            "admission": 0,
        },
        "mixed_time_contact_samples_ms": [milliseconds(r) for r, _ in mixed],
        "limits": [
            "Only two original406 preparations completed; no third406 was requested or observed.",
            "Actual laterSalad25 is not one of the requested nine destinations.",
            "The outer zero-contact observations remain; later equal brackets are not atomic physics snapshots.",
            "The18m attempt did not establish rail contact or validate native18m geometry.",
            "This is ArcadeOFF control/physics evidence, not production animation/HUD/combo or Ollie acceptance.",
        ],
    }


if __name__ == "__main__":
    assert len(sys.argv) == 2, __doc__
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2))
