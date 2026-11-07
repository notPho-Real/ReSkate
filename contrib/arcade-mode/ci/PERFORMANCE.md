# Arcade CI timing and validation report

Baseline captured November 6, 2026, before the isolated CI changes. Source base:
`4cbb0f2d6d29a9cd17b53e1d5af29b6469355e8d` (RC3). RC4 and Trickpedia
were inspected independently; no gameplay changes were merged.

## Measured baseline

The latest 100 repository runs were inspected. Successful push runs, attempt 1,
were used for the workflow summary in [baseline.json](baseline.json). Workflow
elapsed uses created_at to updated_at as a proxy, not CPU or compilation time.
GitHub can update a completed run later: for example RC4 run 42155978931 has a
37m04s updated_at span but its packaging job finished at 26m43s. Selected job
timestamps below are the more precise artifact-availability evidence.
Reruns, failures and still-running workflows are excluded from success medians.

| Branch | Targeted elapsed median | Development elapsed median | Full upstream elapsed median |
|---|---:|---:|---:|
| RC3 | 84.5s (n=24) | 21m10.5s (n=26) | 39m34s (n=21) |
| RC4 | 88.5s (n=2) | 25m38s (n=3) | 38m46s (n=2) |
| Trickpedia | 95s (n=1) | Initially still running | Initially still running |

Trickpedia's first completed sampled development run 42312179189 subsequently
showed a 16m30s build job and 22s packaging job. One run is not a reliable median.
The preceding Trickpedia push was still compiling when that run completed.

Selected successful job samples (RC3: 42307227449, 42302702515, 42298523890;
RC4: 42304893652, 42155978931, 42155898173):

| Measurement | RC3 | RC4 | Trickpedia (one run) |
|---|---:|---:|---:|
| Build job median | 20m49s | 25m03s | 16m30s |
| Runtime/tools build step median | 19m48s | 23m58s | 15m30s |
| Runtime CMake configure median | 23s | 23s | 18s |
| Windows test compilation median | 7m21s | 8m44s | 8m54s |
| Windows test execution median | 23s | 29s | 28s |
| Installer packaging command | 2s | 2s | 2s |
| Packaging job | 21–28s | 24–28s | 22s |

The runtime/tools step includes native compilation, linking and Cargo; it is
not pure C++ compiler time. Cargo can overlap other MSBuild work, so subtracting
its elapsed duration does not establish C++ CPU time. Historical logs separately
report Cargo release completion: RC3 warm examples 70s and 73s; RC4 misses
296s, 304s, 304s; Trickpedia miss 203s. The uncached full integration example
42307231757 reports Cargo 457s, Windows crash preflight compilation 239s,
ALL_BUILD 2042s, CTest 40s and configure 20s.

Runner queue measurement uses each job's created_at to started_at, excluding
skipped jobs and dependency waiting. Selected jobs waited 1–5s. This sample
does not establish an hour-long runner queue. A packaging job's dependency
wait is compilation/test critical-path time, not runner queue time.

Cache log sample: RC3 2/2 compiled-output hits; RC4 0/3; Trickpedia 0/1.
Registry hits: RC3 2/2, RC4 1/3, Trickpedia 0/1. These six selected logs are
not a repository-wide cache rate. RC3 restore steps cost 17–23s, post-cache
steps about 1s; RC4 restore/miss checks cost 13–28s and post-save 5–10s.
The measured Rust saving is minutes versus seconds of cache overhead, although
fresh checkout timestamps can still rebuild the root crate on an exact hit.

Repeated-push analysis counts a run as potentially obsolete when a newer push
to the same branch and workflow began before its updated_at. In the 100-run
sample, RC3 contains 9
such development runs and 11 integration runs.
This is evidence of overlapping work, not proof all those runs were dispensable:
an explicitly requested candidate must be retained. RC4 also had two pushes
45 seconds apart that both completed a roughly 22–27 minute development graph.
Trickpedia additionally duplicated a fast push check with a PR fast check.

## Implemented optimization

- One read-only validation graph runs the targeted tests and existing independent
  Windows/Linux/macOS gates, and packages the exact-source development runtime.
- Ordinary stage pushes omit ALL_BUILD and the upstream integration matrix.
  Ready PRs, explicitly labeled draft candidates, manual checks, protected
  branches and explicit candidate markers retain comprehensive integration.
- Full integration stages its own runtime once. Packaging and eligibility
  consume the immutable same-run artifact rather than compiling another DLL.
- Equivalent ordinary stage validation jobs supersede each other. Workflow,
  job, event and full ref isolate cancellation. Candidates use unique run/attempt
  groups; no parent cancellation wraps packaging, publication or hardware.
- Publication requires the same graph's full release-eligibility gate. Failed,
  cancelled or skipped dependencies cannot qualify a DLL from another run/SHA.
- Cargo dependency caching is available to the full build too. A new v2
  compiled-output namespace includes the cache implementation and exact source.
  SOURCE_DATE_EPOCH uses the checked-out commit timestamp, and librespot-core
  is explicitly cleaned after restore to regenerate source-dependent metadata.
  PR events bypass cache reads/writes. No CMake build tree is cached.
- C++ caching is deliberately not enabled without a supported generator and
  measured clean/cached equivalence. No unmeasured sccache speedup is claimed.

This reduces redundant runner work; it does not by itself establish a faster
single native compilation. The main remaining bottleneck is MSVC compilation
and linking. The under-three-minute fast-feedback target was already met.
Development DLL improvement below the historical 20–26 minutes must be assessed
from completed hosted samples, not inferred from the reduced workflow count.

## Validation evidence and reporting protocol

Local actionlint 1.7.12 passes the actual workflows. Eight policy/provenance tests
exercise the YAML expressions and all negative gate states. Twelve security
tests preserve the read-only graph, exact action pins and publication-only write
permission. With CMake available, the complete local lab suite passed all 459
tests; the 20 standalone native tests also passed. Native negative controls in hosted CI explicitly require
both compiler errors and failing CTest tests to produce nonzero exit status.

Hosted runs are linked in draft PR #3. The first iteration exposed the existing
security test's local-workflow allowlist; it failed closed and packaging was
skipped. The implementation was corrected to keep the reusable graph read-only,
rather than granting validation write permissions. Obsolete ordinary Windows
jobs were actually cancelled on subsequent pushes.

The Cargo audit records restored, immediate warm and clean SHA-256 plus elapsed
time in cargo-audit.json. The initial raw-byte audit failed (run 42314303730): restored/warm output and
the clean archive have different raw hashes. A follow-up audit retains raw hashes
and compares every byte except documented COFF/archive timestamp fields. Unknown
formats are hashed in full; negative tests ensure code/flags/name changes are
still detected. A passing normalized audit would establish content equivalence,
not raw timestamp reproducibility. Raw installer DLL checksums remain unchanged. Immediate warm
is an in-job no-change measurement, not a second hosted cache restore. Retained
Cargo caches must pass that content audit; native outputs remain clean CMake builds.

The diagnostic audit 42316765630 captured both libraries. Six of 6,939 archive
members differ beyond timestamps, including librespot's random build ID
(`KuLRlmHy` versus `cV2iPo7W`) embedded in the root object. The dependency's
[versioned build script](https://github.com/librespot-org/librespot/blob/v0.8.0/core/build.rs)
uses SOURCE_DATE_EPOCH when present and randomness otherwise. The v2 experiment
sets that variable and rebuilds the source-sensitive dependency after restore;
it must pass the unchanged content comparison before being retained. The
comparison does not mask build IDs, code, symbols or string-table differences.

The final optimized timing comparison, audit hashes and exact hosted run links
are recorded in the PR description after validation completes. Recording those
results there avoids another documentation-only commit invalidating the tested
source identity. The final report distinguishes cold cache, a fresh hosted warm
cache, and in-job no-change timing; sample size is always stated.

Actual RC3/RC4/Trickpedia pushes with the new files and two-branch simultaneous
pushes cannot be exercised without changing those active branches. The actual
expressions are tested locally for all three names and interleavings. This
isolated branch cannot prove repository-wide branch protection or external
manual release procedures: maintainers must require the full eligibility check
before promotion. No claim of hardware acceptance or changed hardware policy
is made. The trusted Deck runner, installation flow and publication hook are
unchanged.

Primary semantics: [GitHub concurrency](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency),
[cache access](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching),
[CMake compiler launchers](https://cmake.org/cmake/help/latest/prop_tgt/LANG_COMPILER_LAUNCHER.html).

