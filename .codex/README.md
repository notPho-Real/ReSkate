# Arcade Lab trust boundary

Keep workspace-write, network disabled, and no additional writable roots.
Repository executable paths are mutable and are never persistent approvals.

The separately installed external runner validates bounded JSON from stdin.
Its reviewed worker bundle and target pins live outside the checkout. Updates
require explicit review and digest changes. Plans cannot select code, hosts,
paths, deployment, stock deletion, system changes or shell commands.

Reload the host after rule changes; a running session may retain old rules.
Until reload, use explicit per-action review. No broad Python/shell/SSH approval.

Reference: https://learn.chatgpt.com/docs/agent-configuration/rules

## Source-first gameplay engineering

Before designing or implementing a mechanic, search Arcade Mode, the entire
official upstream repository, relevant surveyed forks and established source
documentation for an equivalent. Identify exact functions, native fields, owners
and observations; classify the defect as missing behavior, routing, timing or
observation. Reuse existing mechanisms unless a demonstrated gap requires more.
Define independent native acceptance evidence and negative controls before HIL
trials. Input requests, synthetic tests and lingering HUD labels never prove an
action; screenshots are secondary. Preserve native physics and trusted HIL.
For rate-dependent trials, verify the clock cadence and the local skater's cached
step with the existing physics observer; a ForceSimRate readback alone is insufficient.
Serialize all Deck input and console operations, including telemetry on/off;
wait for the preceding controller job to complete before starting another.
