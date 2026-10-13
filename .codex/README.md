# Repository-scoped autonomy

The config requests workspace-write, high reasoning and automatic review of
eligible escalation requests, while keeping general network access off and
adding no writable roots. It does not override managed policy or change a
running session. Restart/reload a **trusted** project for this layer and its
rules to load. No HOME/global config or existing user rule is edited.

The installed client was `codex-cli 0.155.0-alpha.9.2`. Project rules use absolute
paths for this checkout deliberately: another repository's same-named script
must not match. Relocation requires reviewing/updating those paths.

Allowlisted commands use `python3 -B` followed by the absolute repository path:

- `lab/trick_batch.py PLAN [--validate-only]`: plan and output confined to repo,
  fixed paired Deck, no deploy/restore/publish, bounded data-only gestures.
- `lab/inspect_project.py status|diff|log|runs|release`: fixed git and fork-only
  GitHub reads; no arbitrary forwarded arguments.
- `lab/inspect_project.py robot-preflight|lab-status|lab-preflight|lab-trace`:
  invokes robot.py/controller.py with a fixed target and no forwarded options.
  Direct CLI prefixes are deliberately excluded because their optional host
  arguments could otherwise escape the established target.

Single-run input/capture and deployment remain eligible for automatic review,
not a blanket SSH/python/shell allow rule. Standing user authorization covers
routine safe development; it does not authorize unrelated files/repos, stock
backup deletion, Skate.exe changes, root or OS changes. Rules are command
exceptions, not a replacement for script validation or the sandbox. Review
script changes before trusting these exceptions.

Sources: https://learn.chatgpt.com/docs/config-file/config-reference and
https://learn.chatgpt.com/docs/agent-configuration/rules .
