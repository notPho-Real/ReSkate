"""Analyze retained native receipts offline; never reads a process or sends input.

Usage: python analyze_grind_readmission.py /absolute/path/to/reskate-arcade-lab
The hashes identify the original receipts, not a gameplay acceptance claim.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def receipt(root, name, digest):
    path = root / name / "results/two-leg-lateral.json.gz"
    data = path.read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, path
    return json.loads(json.loads(gzip.decompress(data))["stdout"])


def analyze(root):
    smith = receipt(root, "grind-native-smith-reference-c048914-v1",
        "6fbed3da9095ade50932c2fa9d997a08db85f0b36c02cc0795f830294a9c15f8")
    rows = smith["native_samples"]
    disagreements = []
    for row in rows:
        bracket = row.get("admission_sample")
        if (row["state"] != 403 or row["named"] != 26 or not bracket
                or not bracket["stable_reads"]):
            continue
        if row["allowed"] != bracket["physical_contact"]:
            assert row["allowed"] == 0 and bracket["physical_contact"] == 1
            assert bracket["state"] == 403 and bracket["named"] == 26
            assert bracket["segment_identity"] == row["active_segment"]["segment_identity"]
            assert bracket["segment_identity"] == [3152505995303, 0]
            assert bracket["current_orientation"] != row["orientation"]
            disagreements.append({
                "time_ms": (row["ns"] - rows[0]["ns"]) / 1e6,
                "outer_contact": row["allowed"],
                "later_bracket_contact": bracket["physical_contact"],
                "same_segment": True,
                "orientation_changed_between_reads": True,
            })
    assert len(disagreements) == 1

    slides = []
    for name, digest, identity in (
        ("grind-native-tail-release-c048914-v1",
         "22d17f6ec2af2d21364e8a7099c1b27a53639255c7631e36ab111e76a3692743", 22),
        ("grind-native-nose-lateral-c048914-v2",
         "a47227610d824030473948b7dc1641d55b67ebde3891e65fdff8194f2d59d3b6", 21),
        ("grind-native-nose-opposite-c048914-v1",
         "eb8fdaa09ee61f5a126c129c7ea6e3920382dddd8d77dc8acf24c3f62a3629da", 30),
        ("grind-native-two-leg-lateral-c048914-v4",
         "06523b2ba624d8c35319db57f3b1f7ba4be5f2092b67d942d58046f375b1471e", 31),
    ):
        data = receipt(root, name, digest)
        rows = data["native_samples"]
        samples = [row["admission_sample"] for row in rows
                   if row["state"] == 402 and row["named"] == identity
                   and row.get("admission_sample")]
        assert samples
        segment = samples[0]["segment_identity"]
        stable = [s for s in samples if s["stable_reads"] and s["state"] == 402
                  and s["named"] == identity and s["physical_contact"] == 1
                  and s["published_contact"] == 1 and s["missing_segment"] == 0
                  and s["segment_identity"] == segment]
        assert stable
        assert all(s["published_admission"] == 0 and s["actor_active"] == 0 for s in stable)
        slides.append({
            "receipt": name,
            "identity": identity,
            "destination_samples": len(samples),
            "stable_same_segment_contact_samples": len(stable),
            "admission_samples": sum(s["published_admission"] != 0 for s in stable),
            "last_requested_heading_pitch_roll": stable[-1]["requested_orientation_raw"],
        })
    return {
        "smith_mixed_read": disagreements,
        "slide_readmission": slides,
        "limits": [
            "Repeated equal reads are not an atomic physics-step snapshot.",
            "The outer Smith zero remains evidence; the later one cannot erase it.",
            "These receipts do not establish a completed-frame contact loss or uninterrupted contact.",
            "No slide-to-grind control sequence was delivered or qualified by this analysis.",
        ],
    }


if __name__ == "__main__":
    assert len(sys.argv) == 2, __doc__
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2))
