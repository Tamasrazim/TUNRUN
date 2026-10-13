# Technical Architecture

## Current implementation

TUNRUN is an actively developed native C++20 gameplay prototype, not a documentation-only shell and not a finished game. The current code includes a raylib window and screen stack, keyboard/mouse/gamepad controls, Windows Raw Input mouse-look, a fixed 16:9 logical canvas, a seeded procedural tunnel, aperture gates and moving mines, flight/camera logic, first-contact collision handling, rewards/scoring, a ship hangar, and a versioned local profile store with validation, migration, checksums, backup recovery, and explicit reset/recovery behavior.

The project has automated C++ regression tests for deterministic generation, input and camera helpers, collision ordering, viewport mapping, hazard motion, progression, and save/profile handling. GitHub Actions builds and tests the native Windows x64 target, runs portable deterministic tests with GCC on Linux, checks repository policy and Markdown links, scans history for secret-looking values, and uploads a temporary Windows development artifact. A passing unit test or build does not replace manual gameplay and hardware validation.

raylib 5.5 is fetched from an immutable upstream commit recorded in `THIRD-PARTY-NOTICES.md`; it is not fetched from a moving branch.

## Platform and build

- Language: C++20.
- Rendering and window/input foundation: raylib 5.5.
- Build system: CMake with Windows x64 / MSVC as the primary game target.
- Linux CI compiles and runs the deterministic test target; this is not a claim of supported Linux gameplay.
- Windows CI produces a temporary portable development package. TUNRUN is not yet a finished release and does not claim a production installer or updater.
- Persistent settings and progression are stored in a versioned local profile, with Windows-specific file handling and bounded parsing.

## Major modules

### Application and state machine

`src/main.cpp` owns the native application loop, drawing, screen transitions, pause behavior, run results, camera presentation, and integration of the gameplay helpers. `src/app/input_state.hpp` provides the screen stack and input/state rules used by that loop. Settings opened from Pause return to the parent Pause screen, and the run clock is gated while pause or blocking dialogs are active.

### Input and presentation

The game supports keyboard and gamepad UI navigation, keyboard/gamepad flight control, pointer-driven menus, and a fixed 1280×720 logical canvas letterboxed inside other aspect ratios. Pointer hit-testing is converted back into logical coordinates. On Windows, `WM_INPUT` supplies relative camera-look deltas while a focused run has mouse-look enabled; capture and clipping are released when gameplay ends or focus is lost. Physical Windows DPI, focus-loss, and resize behavior still require hardware validation.

### Flight, collision, and camera

Flight state and control response are held separately from the rendering camera. First-person and third-person camera rigs share the craft state while changing only the view. Swept wall, gate-throat, gate-plane, and moving-mine contact candidates are compared by movement fraction so the earliest collision determines the run outcome. Collision-time interpolation prevents post-impact movement from being counted in the final state. Manual playability, warning timing, and collision/render agreement still require Windows hardware checks.

### Procedural world and validation

The deterministic course sampler creates a seed-driven tunnel centreline, radius, and twist. Versioned gate and hazard streams produce aperture families, moving mines, pickups, and rendering-only variations. Course and obstacle hashes support reproducibility. Seed Lab performs parameter validation and bounded route-witness searches; a missing witness is not proof that a safe route is impossible. More holdout seeds, broad cross-compiler fixtures, and manual continuity checks remain open.

### Rewards, progression, and saved profile

The prototype has seeded pickups, score/combo tracking, ship-specific handling, a hangar economy using Aether Shards and Singularity Cores, and persistent settings/progression records. The profile parser bounds file size and supported value types, rejects invalid checksums/state, migrates supported legacy schemas, and preserves a known-good backup during recovery. Campaign checkpoints, mode-specific records, and complete progression content remain unfinished.

### Rendering and assets

The renderer uses generated tunnel geometry, low-poly ships and obstacles, depth/fog visibility limits, and deterministic decorative variation. The Windows portable-development packaging script generates twelve original high-resolution UI material sheets and records a SHA-256 manifest. A source checkout can fall back to vector UI panels when baked material assets are absent. Production art/audio, measured frame-time profiling, and license/provenance review for any future external materials remain release work.

## Still planned or incomplete

- Complete campaign/challenge content and broader progression.
- AI pilots, drones, replay/ghost support, and related validation.
- Wider multi-seed and cross-compiler regression fixtures.
- Manual Windows hardware validation for Raw Input, focus handling, DPI, resize/fullscreen hitboxes, gamepad feel, camera clearance, hazard warnings, and long-session stability.
- Release-candidate acceptance tests, clean-machine installation checks, final package verification, and release signing/checksum publication.

## Runtime and security invariants

- UI and gameplay input must not accidentally consume the same action.
- Focus loss must release mouse capture and must not leave flight input stuck.
- The same seed, generator version, and ruleset must reproduce the same gameplay data.
- Cosmetic random streams must not change deterministic gameplay obstacles.
- Asynchronous work must not mutate stale run state or manipulate raylib GPU resources off the render thread.
- Save/profile data, seeds, and future imported replay or asset files are untrusted input.
- Failed save, recovery, purchase, or unlock operations must not be reported as successful.
- The application must not require elevated privileges or commit credentials, private saves, crash dumps, or machine-specific paths.

## Error handling and resource ownership

Invalid settings must fall back to safe defaults; invalid profile files must follow a bounded recovery path instead of silently discarding progression. Generation validation must fail with finite retries and diagnostic information. Owned textures and native input hooks must be released during screen transitions and shutdown. Build output must not depend on the developer's checkout layout.

