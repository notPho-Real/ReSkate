"""Create the exact-source, experimental unified candidate manifest."""
import hashlib
import json
from pathlib import Path
import re
import sys


def create(directory, source_manifest):
    source = json.loads(source_manifest.read_text())
    provenance = json.loads((directory / "provenance.json").read_text())
    features = json.loads((directory / "arcade-build-features.json").read_text())
    fingerprint = (directory / "arcade-gameplay-fingerprint.txt").read_text().strip()
    sha = (directory / "commit.txt").read_text().strip()
    digest = hashlib.sha256((directory / "ReSkate.dll").read_bytes()).hexdigest()
    if not re.fullmatch(r"[0-9a-f]{40}", sha) or not re.fullmatch(r"[0-9a-f]{64}", fingerprint):
        raise ValueError("Invalid candidate identity")
    if provenance["source_sha"] != sha or provenance["dll_sha256"] != digest:
        raise ValueError("Candidate provenance differs")
    if features != {"wallplants": False, "game_build": 20260929}:
        raise ValueError("Pre-RC4 candidate must compile wallplants OFF for build 20260929")
    source.update(source_sha=sha, dll_sha256=digest, gameplay_fingerprint=fingerprint,
                  features=features, run_id=provenance["run_id"], validation=provenance["validation"],
                  hardware_acceptance="pending; inherited evidence is historical, not candidate acceptance")
    (directory / "candidate.json").write_text(json.dumps(source, indent=2) + "\n")
    return source


if __name__ == "__main__":
    create(Path(sys.argv[1]), Path(__file__).resolve().parents[1] / "unified-sources.json")
