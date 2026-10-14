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
