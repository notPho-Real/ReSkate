"""Pinned negative Noseslide return preparation; offline, no hardware access.

Usage: python analyze_grind_noseslide_return.py /absolute/path/to/reskate-arcade-lab
Actual Noseslide on another native segment cannot qualify a retained-segment
return. This recording sent no third-leg controls or feature lease.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def analyze(root):
    path = root / "grind-native-noseslide-return-c048914-v1/results/two-leg-lateral.json.gz"
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == "e0f168365b1b9502cef8d8d604854ca59387f5097e1e543ac513f861065e1d4c"
    envelope = json.loads(gzip.decompress(blob))
    assert envelope["returncode"] == 0
    data = json.loads(envelope["stdout"])
    assert data["error"] == "AssertionError: Native preparation gate failed"
    assert not any("error" in k for k in data if k != "error")
    result = next(json.loads(line.split(" ", 1)[1]) for line in data["gate_log"]
                  if line.startswith("GRIND_GATE_RESULT "))
    assert result["reason"] == "AssertionError: "
    assert result["legs"] == 2
    assert [(c["source"], c["target"]) for c in result["completed"]] == [(401, 403), (403, 402)]
    assert result["first_named"] and not result["second_named"] and not result["third_ready"]
    assert result["third_control_reads"] == [] and "third_ready_reads" not in data
    assert not any(a.get("leg", 0) > 2 for a in result["actor_events"])
    partial = data["partial_run"]
    timing = partial["delivery_timing"]
    assert partial["neutralized"] and timing["interrupted_phase"] == 7
    assert [c["phase"] for c in timing["conditions"]] == [4, 6]
    assert all(c["status"] == "READY" for c in timing["conditions"])
    assert [s["scheduled_ms"] for s in timing["samples"]] == [0, 700, 1100, 1250, 1300, 1500, 1800]
    # No candidate Up/diagonal/neutral writes at2700/2900/3500 were delivered.
    rows = data["native_samples"]
    assert len(rows) == 297
    assert all(abs(r["physics_dt"] - 1 / 30) < 1e-7 and
               abs(r["cached_dt"] - 1 / 30) < 1e-7 for r in rows)
    origin = rows[0]["ns"]
    def ms(row):
        return (row["ns"] - origin) / 1e6
    stable = [(r, b) for r in rows if (b := r.get("admission_sample")) and b["stable_reads"] and
              b["physical_contact"] == b["published_contact"] == 1 and b["missing_segment"] == 0]
    original = [1443109011495, 0]
    offset = [1443109011523, 0]
    prepared = [(r, b) for r, b in stable if b["state"] == 403 and b["named"] == 23]
    nose = [(r, b) for r, b in stable if b["state"] == 402 and b["named"] == 21]
    before_nose = [(r, b) for r, b in stable if b["state"] == 402 and b["named"] == 66]
    assert len(prepared) == 17 and all(b["segment_identity"] == original for _, b in prepared)
    assert len(before_nose) == 3 and all(b["segment_identity"] == original for _, b in before_nose)
    assert len(nose) == 23 and all(b["segment_identity"] == offset for _, b in nose)
    assert all(b["actor"] == 4138024800 and b["actor_active"] == b["published_admission"] == 0 for _, b in nose)
    original_ends = [[145.5234375, .5007421970367432, -43.2109375],
                     [145.5234375, .5007421970367432, -55.2109375]]
    offset_ends = [[145.61004638671875, .4507421851158142, -43.2109375],
                   [145.61004638671875, .4507421851158142, -55.2109375]]
    assert all(r["active_segment"]["endpoints"] == original_ends for r, _ in before_nose)
    assert all(r["active_segment"]["endpoints"] == offset_ends for r, _ in nose)
    assert [r["active_segment"]["segment_changed"] for r, _ in nose] == [1] * 3 + [0] * 20
    assert [ms(nose[i][0]) for i in (0, -1)] == [3358.110188, 3647.727513]
    assert all(r["allowed"] == r["contact"][0] == 1 for r, _ in nose)
    assert not any(b["state"] == 402 and b["named"] == 21 and b["segment_identity"] == original for _, b in stable)
    assert data["gate_returncode"] == 0 and data["gate_restored"] and data["original_fixture_absence_restored"]
    after = data["after"]
    assert after["generation"] == 32 and after["revision"] == 587 and after["arcade"]
    assert not after["present"] and not after["native_in_menu"] and not after["editor_open"]
    return {
        "original_completions": result["completed"],
        "writer_samples": timing["samples"],
        "original_segment": original,
        "original_endpoints": original_ends,
        "different_segment_noseslide": {"identity": offset, "endpoints": offset_ends,
                                         "stable_samples": len(nose),
                                         "reader_ms": [ms(nose[0][0]), ms(nose[-1][0])]},
        "restored_revision": after["revision"],
        "limits": ["Gate assertion site/value was not captured; segment mismatch is independent reader evidence, not a predicate call trace.",
                   "No third-leg input, admission or original completion; no Noseslide return destination qualified.",
                   "Equal sequential brackets are not atomic frames or uninterrupted-contact proof.",
                   "ArcadeOFF control experiment, not integrated production animation/HUD/combo or physical-A acceptance."],
    }


if __name__ == "__main__":
    assert len(sys.argv) == 2, __doc__
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2))
