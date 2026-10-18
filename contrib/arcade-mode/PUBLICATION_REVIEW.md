# Publication review

The trusted local pre-push gate must inspect the exact outgoing range, including
every intermediate commit tree, path, raw commit message and identity metadata.
It also fingerprints the entire index and nonignored tracked/new working content.
Committed generated files receive the same inspection. Ignored local artifacts are
not pushed; adding one places it under inspection. Never publish local audit logs.

Only an existing `origin` development branch fast-forward is supported. The
inspected current local branch must be `arcade-mode` or the fixed private
`arcade-coding` branch; the sole remote destination is `arcade-mode`. New
branches, deletion, tags, rewritten history, missing history, nonstandard object
formats and inspection errors are blocked. New opaque binaries require a separate
reviewed publication procedure; the gate still scans all existing blob bytes for
prohibited naming and private identifiers. Licensed/vendor text is preserved;
generic example paths and reserved documentation IP ranges are permitted.

Before each push, independently review:

1. New/modified source and the complete staged diff.
2. Every outgoing subject, body, author identity, filename and tree.
3. Generated artifacts, fixtures, logs, JSON, scripts and documentation.
4. Private paths, identifying endpoints, credentials and semantic disclosures.
5. GitHub-visible workflows and any proposed PR/issue/release text.

Automated patterns cannot establish privacy clearance. After that review, generate
an inventory with the trusted script's `--inventory` option, providing Git's exact
four-field pre-push update on stdin. Store an external receipt containing precisely
`{"digest":"<inventory digest>","semantic_review_complete":true}` at the path
selected by the trusted hook. Never create this receipt merely because tests pass.
It must follow the independent semantic review. Changed refs/index/working files
invalidate the receipt. Review again if anything changes before publication.

The receipt also binds the trusted scanner, naming helper and installed pre-push
wrapper bytes. Scanner changes require independent code review and a new receipt;
editing a repository copy never updates an installed trusted copy. The installer
itself must be checked against the independently reviewed package before running.
Receipts are local attestations, not signatures or enforced reviewer identities.

Outgoing traversal is `local-tip` minus everything reachable from the supplied
remote tip, including all merge-side ancestors. Git replacement objects are
disabled; shallow or grafted history is rejected. The remote tip must exist and
be an ancestor. The gate rejects more than 512 outgoing commits, blobs over
16 MiB or a combined blob review over 256 MiB. Existing binary bytes are scanned
as Latin-1/UTF-8/UTF-16, but compressed/encrypted content cannot receive semantic
clearance from patterns. Concurrent edits after a scan remain a race: re-review
and serialize publication with writers. No pattern scanner proves absence of
arbitrary identifiers, obfuscation or private context.

Installation must copy reviewed guard bytes and the naming helper outside the
checkout, preserve every existing hook, and add a pre-push entry that executes
the trusted copy with isolated Python. Check all prior hooks' byte hashes before
and after. Do not replace the existing hook directory in place. Verify the hook
rejects a real Git push to a disposable local fixture before enabling it here.

This is a local safeguard. `--no-verify`, another checkout, GitHub API writes,
server-side workflows and direct writers can bypass it. Post-push CI detects
exposure after publication; it is not a server pre-receive gate. Require the same
review process for those writers and repository-side protections where available.
Do not claim this removes prior public exposure or all historical copies.

## Isolated implementation and exact-SHA promotion

Use the private `arcade-coding` worktree. For the initial cleanup, start at the
published baseline and reapply the unpublished evidence documentation delta plus
the path correction in the same reviewed candidate. Preserve the original local
evidence commit and dirty HIL checkout. This avoids publishing an intermediate
tree containing the old path and requires no public history rewrite.

After root reviews the exact private candidate/receipt, the sole supported
promotion refspec is `refs/heads/arcade-coding:refs/heads/arcade-mode`. The gate
requires the inspected checkout's symbolic HEAD and local ref to match that
source and the reviewed local object ID. The remote old ID must be an ancestor;
the receipt binds both refs and IDs. Remote drift invalidates it. Do not merge,
rebase, cherry-pick or amend after exact-SHA review. Promotion changes no content
or SHA and does not edit the dirty HIL checkout. CI then runs for that exact
promoted SHA; root fidelity review and approval precede Ding Bat's exact-SHA HIL.
New local implementation may continue separately while HIL uses an immutable
approved artifact, with every later candidate reviewed again.
