# Bounded logging candidate

Normal logging defaults to warning. Explicit settings and diagnostic command
evidence remain supported; finite diagnostic output is still bounded by retention.
The active `ReSkate.log` and two FIFO archives each have a2 MiB cap. Each write
holds a shared Windows process-session mutex and opens/closes the active path;
launcher/runtime writers cannot retain an obsolete archive handle. Lock waits
are capped at10 ms; contention/failure makes delivery unconfirmed, not successful.
Rotation evicts only the oldest bounded archive. A launcher session rotates the
prior log instead of truncating it. Existing oversized files or partial active
records stop normal file delivery and are preserved for maintenance. This does
not migrate an existing huge log automatically. All participating writers must
use the new binary; an older process does not implement this protocol.

Explicit session records bypass ordinary severity filtering, use PID plus native
process creation time and original observation time, and retain their own two
32 KiB FIFO segments. Launcher records validated file hashes. Runtime records
the backing-file hash for the module selected by the loader, image-validation
completion and Arcade readiness separately. These records never assert native
action acceptance, an installed source commit or a hash of loaded memory. They
must be matched against independently verified deployment pins and current
process generation. A backing-file hash alone does not prove relocated loaded
memory matches disk, especially if a file was replaced after load.

Crash-path records use a separate16 KiB fatal file with exclusive writer access,
size check and no rotation/deletion in the exception handler. Full retention
preserves earlier failures, emits a debugger warning and leaves the independent
crash reporter active. Normal logger rollover cannot erase that failure file.
Total retention for these logger files is at most6 MiB +64 KiB +16 KiB. This
does not cap the separate crash reporter's dump storage or pre-existing files
that this candidate deliberately refuses to erase.

`session_records.py` validates already collected bounded sidecar bytes against
an independently verified active PID/creation time, installation time and module
file hash. Newest runtime generation must match; old prefix records cannot supply
new ready state. Original times are retained; malformed/partial/stale/mismatched
records fail closed. No filesystem/SSH operation or existing reader guard is
changed. It returns fresh_log=false and capture_complete=false intentionally.

## Reader integration and native qualification still required

The installed trusted reader knows only the old unrotated log. It must not be
silently made to accept these sidecars. A separate reviewed protocol and bundle
digest update must collect active process creation time, read complete stable
segments in FIFO order under an aggregate <=16 MiB ledger, validate receipt/source
and installation identity, then use the sidecar validator. Rotation races or
missing segments are explicit loss. Do not invent a new ready line in the main
log, retimestamp old observations, union captures across launches or upgrade
partial capture begin/member/footer evidence. Preserve slot/generation/native
lease and schema3 capture-local owner/poll lineage checks in existing semantic
parsers. Missing lineage remains incomplete. Default warning can omit the old
info-level ready line, so the old reader's readiness result stays unverified.

The C++ regression target exercises the actual portable file writer using local
temporary fixtures, including two writer objects, exact capacity rollover, FIFO
retention, startup boundaries, oversize/partial preservation and sidecar survival.
Build with DINGOSDK_BUILD_TESTS=ON and run CTest `rotating_log`. Windows API lock,
file sharing, crash handler and loaded-module observation require Windows/Proton
compilation and independent multi-process qualification before deployment.

## Existing running writer: maintenance handoff only

There is no current live reopen command. `loglevel warning` reduces ordinary
verbosity but does not remove existing bytes, stop explicit diagnostic commands,
or restore reader admission. `log flush` does not close/reopen the file.
Rename/unlink leaves existing append handles writing into the old file/inode.
In-place truncation loses unresolved evidence and the startup/ready anchor; it
does not establish a fresh session. None is a sound live reset by itself.

For separately authorized one-time maintenance, normally exit both game and
launcher, confirm both are stopped, preserve the unresolved log with restricted
access through an explicitly reviewed archive operation, then normally relaunch
through the existing launcher. The old launcher resets the log before starting
the child. Verify new process, installed/loaded observations, original ready
boundary and guarded reader admission before controller tests. Do not kill a
process or assume launcher reset preserves evidence. No maintenance, artifact
installation, hardware input or trusted-bundle replacement is performed by this
source change.
