Arcade Mode CI turnaround
========================

The full arcade-dev workflow remains the release authority. Windows test
compilation/execution and runtime compilation run on independent Windows-2022
runners. Both must succeed, together with Linux and macOS validation, before
Deck packaging and publication. Runtime artifact upload alone is not acceptance.
Existing selected Windows test targets, CTest expression, artifact SHA/source
checks, immutable-release checks and channel-update safeguards are unchanged.
Separate runners duplicate some dependency work and consume more runner minutes;
measure critical-path time rather than summing the two job durations.

Spotify caches
--------------
The local composite action pins actions/cache v4.3.0 to its full commit.
Registry caches contain only registry/index, cache, src and git/db, checkouts,
scoped by OS, architecture and Cargo.lock. No credential files are cached.
Compiled target outputs additionally require the same Rust/Cargo identities,
MSVC version, runner image, release flags, lockfile, Cargo configuration and
CMake integration. Source changes allow a compatible dependency-output restore.
Cargo always executes build --locked --release --lib and validates fingerprints;
a restored .lib timestamp or cache hit never replaces that validation. A fresh
checkout can rebuild the root crate even on a compatible cache hit. CMake build
trees are not cached. GitHub's branch/ref cache access rules still apply.

Fast iteration
--------------
arcade-fast.yml builds eight existing production-header tests in a standalone
CMake project, with no runtime, Cargo or dependency downloads. It runs on relevant
pushes/PRs and supports manual dispatch. It has read-only permissions and cannot
package/publish/deploy. It never replaces any full release gate. Its concurrency
cancellation applies only to this fast workflow, not the release workflow.
Local equivalent:
  cmake -S contrib/arcade-mode/ci/targeted -B build/arcade-fast
  cmake --build build/arcade-fast --config Release --parallel 4
  ctest --test-dir build/arcade-fast -C Release --output-on-failure --no-tests=error

Optional assets
---------------
CMAKE_CONFIGURE_DEPENDS previously contained directories. MSBuild interpreted
these as missing file dependencies and repeatedly warned/rechecked generation.
OptionalAssets.cmake watches matching actual files plus CONFIGURE_DEPENDS glob
membership, preserving additions/removals and valid absent assets. The fixture
regression checks add/edit/delete/no-op behavior, including in the Windows gate.
Hosted warning counts still need confirmation; this does not promise to remove
all unrelated MSBuild warnings.

C++ compiler cache investigation
-------------------------------
CMake's CXX_COMPILER_LAUNCHER works with Makefiles and Ninja generators, not the
existing Visual Studio generator. Setting sccache there would not cache this
release build. Keep the known-working VS release generator for this candidate.
A separate Ninja + sccache experiment would need an initialized x64 MSVC SDK
shell, equivalent static runtime/Release flags, and an embedded /Z7 debug-info
configuration (CMP0141 NEW, CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded) or
unique /Fd handling. Verify resource generation, Rust dependency ordering,
launcher/tools, full tests and package identities before adopting it. Compare
cold/warm timings and sccache hit/miss statistics; Windows support alone does
not establish compatibility with this project. No C++ cache speedup is claimed.
Primary references:
https://cmake.org/cmake/help/latest/prop_tgt/LANG_COMPILER_LAUNCHER.html
https://github.com/mozilla/sccache/blob/main/README.md
https://github.com/actions/cache/tree/0057852bfaa89a56745cba8c7296529d2fc39830

Benchmark protocol and recorded baseline
---------------------------------------
Baseline source: 03b203d48af99252493e87b62f7e96337c4ff00c
https://github.com/notPho-Real/ReSkate/actions/runs/40811274885
Windows job: 19m06s; configure 25s; tests compile 5m22s; tests execute 9s;
runtime/tools 12m15s, including Cargo release 3m25s. The Windows log contains
346 MSB8064 asset-directory dependency warnings. This is a newer measured
baseline than the owner's earlier approximately 21-minute run.
Baseline has no Cargo output cache; a pre-change hosted warm-output measurement
is unavailable. Do not represent a local no-op build as hosted warm CI.

After publication, retain links/logs for the following same-SHA full runs:
1. First normal run: new cache namespace should miss and seed the caches.
2. Dispatch arcade-dev with benchmark=true, cold_cache=false: warm run.
3. If a controlled forced-cold comparison is needed, dispatch the same SHA with
   benchmark=true, cold_cache=true. This bypasses both cache restore and save.
Benchmark dispatch still runs every gate and packages artifacts, but skips
publication/channel updates. Check toolchain fingerprints match between runs;
runner image changes intentionally invalidate compiled outputs. Record cache
restore/save overhead, Cargo elapsed time, both Windows jobs, configure and
compile durations, warnings, and total critical path through Deck packaging.
Repeat if runner contention makes results inconclusive. Do not deploy benchmark
artifacts automatically. Post-change hosted measurements are pending publication.

Local validation (macOS, AppleClang 15, CMake 3.31.6): standalone test compilation
3.69s fresh / 0.14s no-change; 8/8 tests pass. These are fast-lane smoke timings,
not estimates of Windows runtime or CI speedup.
