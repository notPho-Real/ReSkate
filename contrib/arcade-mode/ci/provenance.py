"""Bind a runtime artifact to this run, source and explicit compiler contract."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

CONTRACT = "windows-2022-vs2022-v143-x64-Release-static-crt-v1"


def identity(directory):
    sha = os.environ["GITHUB_SHA"]
    if not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise ValueError("Expected a complete source SHA")
    if (directory / "commit.txt").read_text().strip() != sha:
        raise ValueError("Artifact source mismatch")
    digest = hashlib.sha256((directory / "ReSkate.dll").read_bytes()).hexdigest()
    if (directory / "ReSkate.dll.sha256").read_text().strip() != digest + "  ReSkate.dll":
        raise ValueError("DLL checksum mismatch")
    return dict(schema=1, source_sha=sha, dll_sha256=digest, contract=CONTRACT,
                run_id=os.environ["GITHUB_RUN_ID"])


def create(directory):
    record = identity(directory)
    toolchain = os.environ["ARCADE_TOOLCHAIN"]
    if not re.fullmatch(r"[0-9a-f]{64}", toolchain):
        raise ValueError("Missing compiler fingerprint")
    record.update(toolchain=toolchain, build_attempt=os.environ["GITHUB_RUN_ATTEMPT"],
                  validation="integration-built" if os.environ.get("FULL") == "true" else "development",
                  accepted=False)
    (directory / "provenance.json").write_text(json.dumps(record, indent=2) + "\n")


def verify(directory, full=False):
    record = json.loads((directory / "provenance.json").read_text())
    for key, value in identity(directory).items():
        if record.get(key) != value:
            raise ValueError("Artifact provenance mismatch: " + key)
    if not re.fullmatch(r"[0-9a-f]{64}", record.get("toolchain", "")):
        raise ValueError("Missing compiler fingerprint")
    if record["toolchain"] != os.environ["ARCADE_TOOLCHAIN"]:
        raise ValueError("Artifact compiler fingerprint mismatch")
    if record.get("accepted") is not False:
        raise ValueError("Build output must not declare acceptance")
    if full and record.get("validation") != "integration-built":
        raise ValueError("Development output is not an integration candidate")
    return record


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=["create", "verify"])
    parser.add_argument("directory", type=Path)
    parser.add_argument("--full", action="store_true")
    args = parser.parse_args()
    if args.operation == "create":
        create(args.directory)
    else:
        verify(args.directory, args.full)
