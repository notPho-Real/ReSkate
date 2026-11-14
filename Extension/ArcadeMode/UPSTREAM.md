# Preparing Arcade Mode for upstream

This is local preparation, not a pull request or a release candidate. Keep new
cleanup changes under `Extension/ArcadeMode/`. The current fork must not be
submitted wholesale as an Arcade-only contribution.

## Build the portable contracts

From the repository root, with CMake 3.24+ and a C++20 compiler:

```sh
cmake -S Extension/ArcadeMode/Test -B build/arcade-extension -DCMAKE_BUILD_TYPE=Release
cmake --build build/arcade-extension --parallel 4
ctest --test-dir build/arcade-extension --output-on-failure
```

On Windows, add `-G "Visual Studio 17 2022" -A x64` to configuration,
`--config Release` to the build, and `-C Release` to CTest.

This entry point builds 34 contract tests from the extension without
the fork's CI project, launcher, runtime DLL, downloads or deployment tools.
It applies the existing strict warning policy to the Arcade test targets. The
flatland tests still link the existing Animation Studio core; this is not an
assertion that Arcade can already be copied onto unmodified upstream.

The tests include the real wallplant-disabled runtime and both development and
release flatland configurations. They do not compile the Windows runtime,
exercise enabled native wallplants, or establish live gameplay acceptance.
The full Windows build and independent native checks remain required.

## Scope audit

The initial comparison uses locally available commits only:

- Official baseline: `355bf71e284ad1c29e32ea81cafdef901d74c513`.
- Development snapshot: `aad36a0f91435c06d188826484be77eed726ed53`.
- 315 changed files are inside this extension; 373 are outside it, including
  236 files under `contrib/arcade-mode/`.

These are tree differences, not a claim that every external change is an Arcade
requirement. The baseline was not refreshed for this audit. Repeat the comparison
against an explicitly selected baseline before preparing any eventual submission:

```sh
git diff --name-only 355bf71e284ad1c29e32ea81cafdef901d74c513 dev -- . ':!Extension/ArcadeMode'
```

The following existing dependencies prevent a directory-only runtime submission:

| Area | Current coupling | Cleanup needed |
|---|---|---|
| Build and lifecycle | Root `CMakeLists.txt`, `cmake/Runtime.cmake`, `Runtime/runtime_init.cpp`, `Runtime/client_tick.cpp` | Own tests and implementation in the extension; identify an upstream-supported registration path before changing shared runtime code. |
| Controller and physics | `Extension/UI/Overlay/input_capture.cpp`, `Extension/Skater/client_noclip.cpp`, `client_source_spawn.h` | Inventory ownership, cancellation and callback contracts; reuse existing APIs without dropping controller/plugin coexistence or OFF restoration. |
| Preferences and world observations | `Extension/Profile/local_profile_*`, `Extension/World/local_population_controls.*`, Park Editor callbacks | Separate mode policy from shared storage and read-only observations. |
| User interface and commands | Overlay/native menu Trickpedia, skater menus, `Extension/Console/commands.cpp` | Keep mode-specific presentation and commands owned by the extension or plugin; retain normal native menu behavior. |
| Native bindings | `Engine/Game/Build/20260929/arcade_*.h`, modified skater and settings helpers | Reuse approved general APIs where available. Keep addresses, offsets and fingerprints in the prescribed build headers. |
| Animation and diagnostics | Modified Animation Studio APIs, camera observation and logging helpers | Identify required capabilities separately from fork-only diagnostics and experiments. |

Launcher fixes, release workflows, lab artifacts and unrelated SDK changes are
not part of the requested Arcade contribution. Their presence in the development
history is not a reason to include them in an eventual PR.

## Architecture decision before runtime refactoring

The baseline's `AGENTS.md` requires general SDK APIs and sandboxed Luau plugins
for modes. Its existing plugin services include permission-gated controller,
physics, launch and trick observations; those are the starting point for a
capability comparison, not proof of full Arcade parity.

For each required operation, record the existing upstream API, native owner,
timing and independent acceptance evidence. Any missing general capability is a
separate architecture question. Do not silently expand this cleanup into shared
SDK changes, move addresses into feature code to hide the dependency, or expose
raw memory/console access through the plugin sandbox.

Until those dependencies are resolved, the portable tests provide a regression
baseline while the playable fork and separate wallplant development retain their
current behavior. No upstream PR should be created from this preparation alone.
