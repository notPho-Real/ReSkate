# Arcade Mode security checkpoint

2026-10-14. Hardware acceptance and signing verification are separate from local
tests. This document does not claim completion of gameplay milestones.

## Fixed

- Active GitHub rulesets 25060151, 25060152 and 25060153 protect `main` and
  `arcade-mode`. Force pushes and deletion have no bypass. Updates require the
  repository admin role. Main requires pull requests and code-owner review;
  the sole owner retains an audited admin emergency bypass.
- Build/test/package jobs retain read-only contents. Only the separate publish
  job receives write authority, gated to the fork's `arcade-mode` push/manual
  runs after all test/package jobs pass. It checks out no repository code.
  Retries compare both immutable published files byte for byte. Source and DLL
  checks remain in the package job; latest-channel publication checks branch head.
- Checkout, upload and download Actions use full official commit pins with
  readable version comments. Checkout does not persist credentials.
- CODEOWNERS assigns sensitive surfaces to the actual fork owner.
- Removed repository executable approval rules and 154 project-related local
  persistent approvals. Kept workspace-write, network disabled and no extra
  writable roots. Host policy must reload before removed rules disappear from
  an already-running session; use per-action review until then.
- Installed an independent worker bundle under the user-owned external lab
  directory, `trusted-v1`. The entry point verifies its versioned file manifest,
  takes JSON on stdin, rejects unknown fields/operations, pins the paired host
  and repository, and compares plan hashes with separately reviewed target pins.
  No checkout imports, arbitrary commands, deployment, restore, backup deletion,
  game replacement or root/system operations are exposed by the runner.
- A dedicated SSH commit-signing key lives outside the repository. Local Git
  signing is enabled only for this checkout. External pre-commit and commit-msg
  hooks scan the entire index, filenames, branch and message for prohibited names.

## Verified

- GitHub effective branch-rule GETs confirm enforcement on both target branches.
- Collaborator enumeration: `notPho-Real` is the sole admin, with inherited
  maintain/write/triage/read access. No other collaborators, teams or deploy keys.
  No unexpected write authority was visible in these API results.
- Repository Actions defaults are read-only and cannot approve PR reviews.
- The external runner successfully performed read-only Deck status. Installed
  source `08ab7dc5b0a91c3fcb65ae1e1084fefa427c62a5`, development DLL and stock
  backup match prior recorded pins. Game executable SHA-256 is now independently
  pinned in local trust data. Skate remained running; no restore or restart.
- Python security/installer/lab tests pass locally. Regression cases cover
  permission isolation, action pins, command/path/host rejection, unknown nested
  fields, numeric bounds, duplicate/nonfinite JSON, target hashes and naming.
- Tracked-tree scan found no private keys or recognized service-token patterns.
  Private address hits are synthetic test fixtures; existing image assets are
  product assets. Local tools, captures, raw logs and credentials remain ignored.
  This is a bounded pattern audit, not a guarantee against every secret format.

## Requires account action

1. GitHub account Settings → SSH and GPG keys → New SSH key → **Signing key**.
   Add the public `id_ed25519.pub` from the external lab's `signing` directory.
   Never add or share the private file. Current CLI token lacks signing-key admin
   scope. Then verify the pushed test commit displays **Verified** before enabling
   `required_signatures` on either branch. Keep the owner workflow viable.
2. Account Settings → Applications: inspect installed GitHub Apps and authorized
   OAuth apps with access to this fork; remove unexpected contents/admin write
   grants. The CLI installation endpoint rejected this token type (403), and the
   connector returned no installations. Neither result proves there are no apps.

No manual collaborator check is needed: that enumeration succeeded.

## Remaining risk and next work

An admin account or broad owner token can change rules/workflows. Admin bypass is
retained for the owner development path; this is not a defense against owner
account compromise. CODEOWNERS alone does not prevent admin direct pushes.

The external runner is a versioned snapshot: edits in the checkout do not update
it. Review every change, install a new version/digest and update target pins
explicitly. Never auto-copy workers after pull. Current runner preserves existing
bounded trial behavior; persistent controller/telemetry is milestone 6, not yet
implemented. Routine HIL cannot deploy; a new DLL remains separately reviewed.

Signing enforcement is deliberately OFF until a pushed commit is GitHub Verified.
The live toggle and all later gameplay gates are still pending.

Official references: [rulesets](https://docs.github.com/en/rest/repos/rules),
[local execution rules](https://learn.chatgpt.com/docs/agent-configuration/rules).
