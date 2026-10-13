"""Narrow read-only inspection entry point for project-local Codex rules."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def command(operation):
    git = ["git", "--no-pager", "-C", str(ROOT)]
    gh = ROOT / "tools/arcade-github/gh"
    commands = {
        "robot-preflight": ["python3", "-B", str(ROOT / "contrib/arcade-mode/lab/robot.py"), "preflight"],
        **{"lab-" + op: ["python3", "-B", str(ROOT / "contrib/arcade-mode/lab/controller.py"), op]
           for op in ("status", "preflight", "trace")},
        "status": git + ["status", "--short", "--branch"],
        "diff": git + ["diff", "--no-ext-diff", "--no-textconv"],
        "log": git + ["log", "-10", "--oneline"],
        "runs": [str(gh), "run", "list", "--repo", "notPho-Real/ReSkate", "--branch", "arcade-mode", "--limit", "6", "--json", "databaseId,headSha,status,conclusion,workflowName,url"],
        "release": [str(gh), "release", "view", "arcade-deck-latest", "--repo", "notPho-Real/ReSkate", "--json", "tagName,targetCommitish,url,assets"],
    }
    return commands[operation]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("status", "diff", "log", "runs", "release", "robot-preflight", "lab-status", "lab-preflight", "lab-trace"))
    args = parser.parse_args()
    env = dict(os.environ, GH_CONFIG_DIR=str(ROOT / "tools/arcade-github/config"), GIT_OPTIONAL_LOCKS="0")
    subprocess.run(command(args.operation), cwd=ROOT, env=env, check=True, timeout=30)
