# Technical Architecture

## Proposed platform

- Language: C++20.
- Rendering and window/input foundation: raylib 5.5.
- Build: CMake with a Windows x64 toolchain.
- Primary deliverable: native Windows executable, installer, and portable package.
- Persistence: local versioned JSON profile with Windows-native atomic write/hidden-file handling.
- Automated build/test: GitHub Actions on Windows.

The chosen runtime must support a full native 3D scene, real collision, controller polling, audio, and a Windows package without requiring the player to install development tools.

## Major modules

### Application and state machine

Owns lifecycle, window, mode transitions, pause, focus loss, loading, results, shutdown, and error recovery. State transitions must be explicit and testable. A button must not directly mutate unrelated game systems.

### Input service

Translates raw keyboard, mouse, and gamepad data into actions such as Steer, Boost, Dash, Pause, Confirm, Back, Navigate, and CameraSwitch. UI and gameplay consume the action layer rather than reading devices independently.

### Flight simulation

Owns craft acceleration, forward speed, steering limits, angular response, boost, hull state, and collision response. Rendering camera state must never decide physical flight state.

### Camera system

Implements FPP and TPP as separate camera rigs reading the same craft transform. Camera switching blends presentation only; it does not teleport the ship or modify the course.

### Procedural world service

Produces deterministic section descriptions from root seed, generator version, section index, ship profile, and ruleset. It does not own rendering objects.

### Generation validator

Validates geometry, seams, player reachability, ship clearance, warning time, and dynamic obstacle feasibility before content is accepted.

### Streaming manager

Requests new sections ahead of the player, tracks worker jobs, uploads meshes on the render thread, retains sections needed by rewind/ghost replay, and unloads obsolete content safely.

### Obstacle simulation

Advances generated obstacle behaviours from deterministic section-local time. It supplies collision shapes and warning state to physics and UI.

### AI navigation

Builds navigation options from the same validated route space, predicts obstacle motion, and produces legal steering actions. AI never changes world collision or teleports to a better position.

### Economy and progression

Owns unlock rules, resource transactions, stage records, achievements, and rewards. All transactions are validated and saved consistently.

### Save service

Validates and migrates profile schema, performs atomic saves, maintains backup recovery, stores settings and progression, and handles corrupt/unreadable files without crashing the game.

### UI and hangar

Owns layout, widgets, navigation focus, pointer hit-testing, dialogs, ship preview, settings, and controller prompts. UI widgets call high-level commands instead of directly altering save structures.

### Audio and visual effects

Consumes gameplay events without affecting generation determinism. Particles and cosmetic effects use separate random streams and can be reduced for performance/accessibility.

## Runtime flow

1. Read platform and input events.
2. Resolve active input context.
3. Apply UI actions or gameplay actions, never both accidentally.
4. Advance fixed/controlled simulation steps.
5. Advance craft physics, obstacle motion, and AI.
6. Request and validate upcoming sections from the streaming manager.
7. Resolve collision and gameplay outcomes.
8. Render the selected camera and UI.
9. Process queued save transactions at defined safe points.

Simulation time must be separate from render frame time so high-refresh displays do not alter movement or obstacle timing. Rendering can run at the display rate; physics and deterministic moving obstacles use a controlled timestep.

## Threading boundaries

Workers may build pure CPU generation data and validate immutable snapshots. They must not access raylib GPU resources or mutable live gameplay entities. Mesh/texture upload and destruction occur on the render thread. Worker results are accepted only if their run token and section index are still current, preventing stale jobs from an abandoned run from appearing in the next one.

## Error handling

- Invalid settings fall back to safe defaults and are reported without exiting.
- Corrupt saves attempt backup recovery, then offer a clear reset path.
- A failed generation attempt is logged with seed and section index; bounded retries and a conservative generated fallback prevent an infinite loop.
- Lost window focus pauses and releases gameplay mouse capture.
- Disconnected controllers are removed from active polling without leaving stuck button states.
- Failure to write a save is visible to the player and retried safely; the game must not claim the save succeeded when it did not.

## Build and packaging boundaries

Source, tests, procedural definitions, and development tools remain separate from runtime data. The installer bundles all required runtime dependencies. The game must not rely on an absolute development path, a pre-existing build directory, or tools installed on the developer's PC.

A portable package should use the same application and save path as the installer. Uninstalling should not silently erase player progress; any save removal must be a separate, clearly described user choice.
