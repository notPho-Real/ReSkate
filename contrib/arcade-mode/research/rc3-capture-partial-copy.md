# Windows capture failure during RC3 integration

Integration run 42256080100 at source d0d95ca118a5fd82e1af8c48d3c40c2aa9b85eed compiled every configured target. The early crash-report fixture passed; the full suite passed 122 of 123 tests. Its second crash-report fixture failed saving the live stuck-load dump. The new diagnostic reported `source=2 stage=write windows_error=2147942699`, or HRESULT `0x8007012b` (`ERROR_PARTIAL_COPY`). The following real crash was captured successfully. No candidate from this failed run was installed.

The capture client retains its exception/context stack until the helper acknowledges the request. The helper signals completion after `save_dump`; an acknowledgment alone does not mean a saved dump. This failure is in `MiniDumpWriteDump`, before flushing or publishing, rather than the fixture's search for a filename.

[Microsoft's API documentation](https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump) specifies an HRESULT in the last-error value. The [Microsoft .NET dump collector](https://github.com/dotnet/diagnostics/blob/main/src/Tools/dotnet-dump/Dumper.Windows.cs) also retries this specific partial-copy error. Our implementation permits at most three writes with the original flags and exception. Before another attempt, it rewinds and truncates the unpublished temporary file. A different error, failed reset or persistent partial copy still fails capture. No missing-memory option or weaker dump flags are introduced.

The production retry helper is exercised by the crash-report fixture: immediate success, recovery after one/two partial reads, exhaustion, unrelated errors and cleanup failure. Those same checks pass with the host C++20 compiler under warnings as errors. Actual Windows capture and the complete integration suite still require CI; host checks do not establish Windows success.

## Stable live-process capture

Integration42321457330 at e325137e failed its early fixture with the same
partial-copy HRESULT despite the bounded retries. Its later full suite passed
123/123. A single workflow retry also failed the early capture gate; its exact
diagnostic is pending while that job finishes. Retrying the gate again without
a code change would not address the observed intermittent capture failure.

The helper now requests a Windows PSS snapshot only for a stuck-load capture.
The snapshot retains original thread contexts and a VA clone while DbgHelp
reads them. This follows the [documented Windows snapshot dump mechanism](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/proc_snap/export-a-process-snapshot-to-a-file).
The exact unreadable region in the earlier failure is unknown; this is a
mechanism-level correction for changing live memory, not proof of that region.
Normal fatal/unhandled captures keep their existing process path. PSS APIs
are dynamically resolved, and unavailable snapshots retain live capture with
an explicit metadata error. No missing-memory flag, skipped failed block,
different dump contents, test exclusion or reduced CI gate is introduced.

The Windows fixture requires an actual snapshot, the original exception,
nonempty instruction context, other threads, and stack-referenced heap bytes.
It still requires the following real crash and all existing report metadata.
Callback checks reject ignoring a read failure or taking over file/memory I/O.
Windows execution remains pending CI; this macOS host cannot validate PSS.
