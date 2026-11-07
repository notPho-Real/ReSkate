Arcade CI validation and performance
===================================

Three levels
------------
Every push to arcade-stage-* (including RC3, RC4 and Trickpedia) runs arcade-dev.
The targeted job calls arcade-fast and compiles the existing standalone native
tests immediately. It now runs on all development changes, including Engine
headers and build settings previously missed by the fast workflow path filter.
The independent Windows preflight, Linux contracts and macOS simulations remain
required. No gameplay source or hardware runner is changed.

The development build compiles the runtime, recovery launcher and scoring tool.
The existing artifact names, commit.txt, checksum files, installer, channel.json
and rollback format are preserved. A new provenance.json labels the DLL
development or integration-built; neither label declares hardware acceptance.
Deck packaging downloads this run's exact artifact and checks source, checksum,
run ID and compiler contract. Artifact upload alone is not a release gate.

Complete integration runs for ready-for-review PRs targeting main, arcade-mode or arcade-stage-*,
draft PRs explicitly labeled full-integration, release-candidate or milestone,
manual development/integration dispatches, arcade-rc-* / arcade-milestone-* tags,
main and arcade-mode pushes, and pushes containing [full integration] or
[release candidate]. Explicit markers are case-insensitive GitHub expressions.
Use a manual dispatch or one of these markers for a milestone, not an informal
commit title. Manual dispatch always performs full integration, even if its
legacy full_integration boolean is unchecked.

Full integration is one build graph: arcade-dev and arcade-integration call the read-only arcade-validation graph
at the same revision; only arcade-dev owns the guarded publication job. The build job enables all previous upstream groups, runs the
crash test three times, builds ALL_BUILD, runs all hosted upstream CTest tests,
and stages that very runtime. Independent Arcade Windows preflight and targeted
tests still run. The existing trainer_tuning_names exclusion requires installed
game data; the pre-existing manual differential exclusion is unchanged.
Linux server/portable tests also remain required. This replaces the duplicate
development runtime plus upstream runtime builds with one full runtime build.

release-eligibility requires success (not skipped, cancelled or failed) from
every dependency and verifies the exact packaged runtime again. Publication
also requires this gate. No workflow_run, latest-success query or cross-run
artifact search can accidentally qualify another SHA. PR evidence is for the
merge SHA; it does not qualify a different branch-head SHA. Release eligibility
does not replace owner approval or trusted hardware acceptance. Configure branch
protection to require the complete release-eligibility check before promotion;
this PR does not modify repository rules or owner permissions.

Supersession
------------
Only ordinary stage-push validation jobs share a replaceable group. Keys include
workflow, job/validation level, event and full branch ref. RC3, RC4 and Trickpedia
cannot cancel each other. Explicit candidates, PR integration, manual dispatches,
tags and protected branches use run ID plus attempt, so even pending candidates
cannot be displaced by the default one-pending-run concurrency rule.
No parent workflow cancellation surrounds packaging or publication. Packaging,
release eligibility and the unchanged publication lock remain non-cancelable.
No hosted job installs a DLL or operates hardware. GitHub does not guarantee
queue ordering or available runner capacity; distinct candidate groups prevent
supersession, not all platform-capacity waits.

Cache boundaries
----------------
Spotify keys include compiler/Cargo/MSVC/image/flags/config/lock, the cache
implementation and exact source SHA, with a compatible dependency fallback.
Cargo must always validate fingerprints; a cache hit is never a build
success. Only registry data and SpotifyHelper/target are cached. Credentials,
CMake trees, generated headers and complete DLLs are never compilation caches.
PR events bypass cache reads and writes, keeping fork code out of trusted cache
population. cold_cache bypasses both restore and save on manual full runs.
The toolchain fingerprint is now retained in runtime provenance.
SOURCE_DATE_EPOCH is the checked-out commit's timestamp. This replaces
librespot's random build ID with reproducible source metadata. After a compiled
cache restore, librespot-core is explicitly cleaned so Git/date metadata and
dependent crates are regenerated for this revision. Other dependency outputs
can still be reused. The v2 namespace cannot restore older random-ID caches.

arcade-ci-audit.yml is a read-only, hosted Windows benchmark. It compares
restored-cache, immediate warm and clean Cargo release libraries by SHA-256,
records durations and fails on any content mismatch. Both raw hashes and hashes
excluding only documented archive/COFF timestamp fields are retained; no library
bytes used by the build are changed. Unknown object formats are hashed in full.
This verifies content equivalence, not byte-for-byte reproducibility of timestamped
archives. The checksum used for installation is always the raw DLL SHA-256. Automatic audit runs are limited
to changes to the audit itself on ci-optimization/arcade. It never installs,
publishes or marks a candidate accepted.

C++ caching was investigated but is not enabled: CMake compiler launchers do not
support the current Visual Studio generator. A Ninja/MSVC/sccache migration needs
separate cold/warm DLL equivalence, generated-header invalidation and full
regression measurements. There is no claimed C++ cache benefit in this change.
The current generator, warning-as-error policy and static CRT are retained.

Validation and measurements
---------------------------
baseline.json contains sanitized workflow summaries and selected job/step timings.
PERFORMANCE.md records sample sizes, limitations and optimized hosted results.
Local scheduling tests evaluate the expressions from the actual YAML, including
all three branches, rapid commits, parallel branches, manual/candidate isolation,
workflow/level isolation and every failed/skipped dependency. Provenance tests
reject another SHA, run, contract, changed bytes and development-only acceptance.
Run:
  python -m pip install PyYAML==6.0.2
  python contrib/arcade-mode/ci/test_policy.py -v
  actionlint -shellcheck= -pyflakes= .github/workflows/*.yml

When applying this CI change to RC4 or Trickpedia, preserve their added Windows
preflight targets/CTest selections and standalone targeted CMake lists during
conflict resolution. The optimization is based on RC3 and contains no gameplay
merge from either branch.

Primary references
------------------
https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency
https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching
https://cmake.org/cmake/help/latest/prop_tgt/LANG_COMPILER_LAUNCHER.html
https://github.com/mozilla/sccache/blob/main/README.md
