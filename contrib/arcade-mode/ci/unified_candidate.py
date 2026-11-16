"""Create the exact-source, experimental unified candidate manifest."""
import hashlib
import json
from pathlib import Path
import re
import sys


def expected_features(source):
    enabled = source.get("sources", {}).get("rc4", {}).get("enabled")
    if type(enabled) is not bool or source.get("game_build") != 20260929 or source.get("accepted") is not False:
        raise ValueError("Experimental candidate must declare its RC4 gate and supported build")
    return {"wallplants": enabled, "game_build": 20260929}


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
    if features != expected_features(source):
        raise ValueError("Compiled features differ from the candidate source manifest")
    source.update(source_sha=sha, dll_sha256=digest, gameplay_fingerprint=fingerprint,
                  features=features, run_id=provenance["run_id"], validation=provenance["validation"],
                  hardware_acceptance="pending; inherited evidence is historical, not candidate acceptance")
    (directory / "candidate.json").write_text(json.dumps(source, indent=2) + "\n")
    return source


if __name__ == "__main__":
    manifest = Path(__file__).resolve().parents[1] / "unified-sources.json"
    if sys.argv[1:] == ["--wallplant-cmake-option"]:
        features = expected_features(json.loads(manifest.read_text()))
        print("-DDINGOSDK_ARCADE_WALLPLANTS=" + ("ON" if features["wallplants"] else "OFF"))
    else:
        create(Path(sys.argv[1]), manifest)
