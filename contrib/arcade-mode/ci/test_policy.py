"""Exercise actual workflow expressions and the artifact boundary."""
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace as NS

import yaml
import provenance

ROOT = Path(__file__).resolve().parents[3]


def workflow(name):
    # BaseLoader preserves the YAML 1.2 key 'on' and expression strings.
    return yaml.load((ROOT / ".github/workflows" / name).read_text(), Loader=yaml.BaseLoader)


DEV = workflow("arcade-dev.yml")
FAST = workflow("arcade-fast.yml")
INTEGRATION = workflow("arcade-integration.yml")


def context(branch="arcade-stage-rc3-gameplay", event="push", message="", run=1, full=False, name="Arcade Mode development"):
    return dict(github=NS(workflow=name, event_name=event, ref="refs/heads/" + branch,
                          run_id=run, run_attempt=1,
                          event=NS(head_commit=NS(message=message))),
                inputs=NS(full_integration=full))


def expression(source, ctx):
    source = source.strip().removeprefix("$"+"{{").removesuffix("}}").strip()
    source = source.replace("&&", " and ").replace("||", " or ")
    source = re.sub(r"!(?!=)", " not ", source)
    return eval(source, {"__builtins__": {}, "true": True, "false": False,
                        "contains": lambda s, part: part.lower() in s.lower(),
                        "startsWith": lambda s, part: s.lower().startswith(part.lower()),
                        "format": lambda pattern, *args: pattern.format(*args)}, ctx)


def group(job, ctx):
    return re.sub(r"\$\{\{(.*?)\}\}", lambda m: str(expression(m.group(1), ctx)),
                  job["concurrency"]["group"])


class SchedulingTests(unittest.TestCase):
    def jobs(self):
        return [DEV["jobs"][n] for n in ("build", "windows-tests", "linux-tests", "macos-lab")] + [FAST]

    def test_three_development_branches_and_rapid_pushes(self):
        for job in self.jobs():
            groups = []
            for branch in ("arcade-stage-rc3-gameplay", "arcade-stage-wallplant-rc4", "arcade-stage-trickpedia"):
                contexts = [context(branch=branch, run=r) for r in (1, 2, 3)]
                self.assertEqual(len({group(job, c) for c in contexts}), 1)
                self.assertTrue(all(expression(job["concurrency"]["cancel-in-progress"], c) for c in contexts))
                groups.append(group(job, contexts[0]))
            self.assertEqual(len(set(groups)), 3)

    def test_protected_candidates_cannot_be_replaced_even_while_pending(self):
        for job in self.jobs():
            for kwargs in (dict(message="[release candidate]"), dict(message="[full integration]"),
                           dict(full=True), dict(event="workflow_dispatch"), dict(event="pull_request"),
                           dict(branch="main"), dict(branch="arcade-mode")):
                a, b = context(**kwargs, run=1), context(**kwargs, run=2)
                self.assertFalse(expression(job["concurrency"]["cancel-in-progress"], a))
                self.assertNotEqual(group(job, a), group(job, b))
                self.assertNotEqual(group(job, a), group(job, context(run=3)))

    def test_workflows_and_validation_levels_do_not_collide(self):
        groups = [group(j, context(name=w)) for w in ("Arcade Mode development", "Arcade upstream integration")
                  for j in self.jobs()]
        self.assertEqual(len(set(groups)), len(groups))
        self.assertNotIn("concurrency", DEV)
        self.assertNotIn("concurrency", DEV["jobs"]["deck"])
        self.assertEqual(DEV["jobs"]["publish"]["concurrency"]["cancel-in-progress"], "false")

    def test_full_policy_and_triggers(self):
        policy = DEV["jobs"]["policy"]["steps"][0]["env"]["FULL"]
        for branch in ("arcade-stage-rc3-gameplay", "arcade-stage-wallplant-rc4", "arcade-stage-trickpedia"):
            self.assertFalse(expression(policy, context(branch=branch)))
        for kwargs in (dict(event="workflow_dispatch"), dict(event="pull_request"), dict(full=True),
                       dict(message="[release candidate]"), dict(message="[full integration]"),
                       dict(branch="arcade-mode"), dict(branch="main")):
            self.assertTrue(expression(policy, context(**kwargs)))
        self.assertIn("arcade-stage-*", DEV["on"]["push"]["branches"])
        self.assertNotIn("branches", INTEGRATION["on"]["push"])
        self.assertEqual(INTEGRATION["jobs"]["complete"]["with"]["full_integration"], "true")
        self.assertIn("pull_request", INTEGRATION["on"])
        self.assertIn("workflow_dispatch", INTEGRATION["on"])

    def test_failure_or_skipped_gate_blocks_release(self):
        gate = DEV["jobs"]["release-eligibility"]
        expected = {"policy", "targeted", "build", "windows-tests", "linux-tests", "macos-lab", "deck", "linux-integration"}
        self.assertEqual(set(gate["needs"]), expected)
        script = gate["steps"][0]["run"].split("<<'PYCODE'\n")[1].rsplit("\nPYCODE", 1)[0]
        for state in ("failure", "cancelled", "skipped"):
            for name in expected:
                needs = {n: {"result": "success"} for n in expected}
                needs[name]["result"] = state
                with patch.dict(os.environ, RESULTS=json.dumps(needs)):
                    with self.assertRaises(AssertionError):
                        exec(script, {})
        self.assertIn("release-eligibility", DEV["jobs"]["publish"]["needs"])
        self.assertIn("targeted", DEV["jobs"]["deck"]["needs"])
        self.assertNotIn("continue-on-error", json.dumps(DEV))
        self.assertNotIn("workflow_run", DEV["on"])


class ProvenanceTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)
        (self.path / "ReSkate.dll").write_bytes(b"test fixture only")
        (self.path / "commit.txt").write_text("a" * 40 + "\n")
        digest = hashlib.sha256(b"test fixture only").hexdigest()
        (self.path / "ReSkate.dll.sha256").write_text(digest + "  ReSkate.dll\n")
        env = patch.dict(os.environ, GITHUB_SHA="a" * 40, GITHUB_RUN_ID="42",
                         GITHUB_RUN_ATTEMPT="1", ARCADE_TOOLCHAIN="b" * 64, FULL="true")
        env.start()
        self.addCleanup(env.stop)
        provenance.create(self.path)

    def test_matching_source_and_hash(self):
        self.assertEqual(provenance.verify(self.path, full=True)["source_sha"], "a" * 40)

    def test_different_sha_run_contract_and_bytes_rejected(self):
        original = (self.path / "provenance.json").read_text()
        for field in ("source_sha", "run_id", "contract", "dll_sha256", "toolchain", "accepted"):
            record = json.loads(original)
            record[field] = "incorrect"
            (self.path / "provenance.json").write_text(json.dumps(record))
            with self.assertRaises(ValueError):
                provenance.verify(self.path)
        (self.path / "provenance.json").write_text(original)
        (self.path / "ReSkate.dll").write_bytes(b"modified")
        with self.assertRaises(ValueError):
            provenance.verify(self.path)

    def test_development_build_is_not_release_eligible(self):
        with patch.dict(os.environ, FULL="false"):
            provenance.create(self.path)
        with self.assertRaises(ValueError):
            provenance.verify(self.path, full=True)


if __name__ == "__main__":
    unittest.main()

