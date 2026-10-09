# Technical Architecture

## Current implementation slice

The first source slice provides a raylib-based native window, explicit screen stack, pointer/keyboard/basic-gamepad menu navigation, preview settings, a wireframe 3D tunnel testbed, and pure-C++ input-state unit tests. This is scaffolding, not a finished flight model: collision, procedural generation, persistence and true relative mouse steering are not implemented yet.

raylib 5.5 is fetched from an immutable upstream commit recorded in `THIRD-PARTY-NOTICES.md`; it is not fetched from a moving branch.

## Platform

- Language: C++20.
- Rendering and window/input foundation: raylib 5.5.
- Build: CMake with Windows x64 / MSVC.
- Primary deliverable: native Windows executable, installer, and portable package.
- Persistence target: versioned local profile with Windows-native atomic write/hidden-file handling.
- CI: GitHub Actions on Windows, triggered by direct pushes to `main`.

## Major modules

### Application and state machine
Owns window lifecycle, screen transitions, pause, focus loss, loading, results, shutdown and error recovery. Screen navigation uses a stack so Settings opened from Pause returns to Pause.

### Input service
The shell currently reads raylib input APIs directly. This is temporary. The required next step is a named-action service for Confirm, Back, Navigate, Steer, Boost, Dash, Pause and CameraSwitch, with explicit UI and gameplay contexts. Relative mouse steering must use Windows Raw Input (`WM_INPUT`) or an equivalent native relative-motion path, never repeated `SetCursorPos` warping.

### Flight simulation
Will own craft acceleration, speed, steering limits, angular response, boost, hull state and collision response. Rendering camera state must never decide physical flight state.

### Camera system
Will implement FPP and TPP as separate camera rigs reading the same craft transform. Camera switching blends presentation only; it does not teleport the ship or change the course.

### Procedural world service
Produces deterministic section descriptions from root seed, generator version, section index, ship profile and ruleset. It does not own rendering objects.

### Generation validator
Validates geometry, seams, player reachability, ship clearance, warning time and dynamic obstacle feasibility before content is accepted.

### Streaming manager
Requests new sections ahead of the player and validates immutable data on workers. Workers must not manipulate raylib GPU resources or mutable live gameplay entities. Mesh/texture operations remain on the render thread.

### AI navigation
Builds route options from the same validated course and produces legal steering actions. AI never bypasses world geometry or teleports to a better position.

### Save service
Will validate and migrate profile schema, write atomically, maintain a backup, and offer recovery without crashing or silently discarding progression.

## Runtime invariants

- UI input and gameplay input cannot act on the same event accidentally.
- Focus loss pauses flight and clears held input/captured relative mouse state.
- Same seed, generator version and ruleset reproduces the same underlying course.
- Cosmetic random streams do not alter deterministic obstacles.
- Every async result carries a run/generation token to prevent stale results from changing a restarted run.
- User-controlled saves, seeds, replay data, paths and archives are untrusted input.
- Save/award/purchase operations validate state, commit once and only then report success.

## Error handling
Invalid settings return to safe defaults; invalid files have bounded parsers and a clear recovery path; generation failures have finite retries and diagnostics. Failed saves must not be reported as successful. Disconnecting a controller must clear held actions.

## Security and build boundaries
Do not require elevated privileges. Prefer RAII, standard containers, checked size arithmetic and explicit ownership. Apply supported MSVC security checks and executable mitigations, and verify their presence in compiler/linker output. Never commit keys, tokens, private saves, crash dumps or machine-specific paths. Build outputs must not depend on a developer's checkout.
