# QA and Acceptance Plan

A feature is not complete because its screen exists. It is complete only when its acceptance criteria pass in a real build.

## 1. Input and mouse regression matrix

Test on Windows at normal DPI and scaled DPI, windowed, borderless, fullscreen, after resizing, and after Alt+Tab.

For each screen (Main Menu, Hangar, Mode Select, Seed Entry, Settings, Pause, Results, confirmation dialogs):
- Every button is mouse-clickable at its visible position.
- Hover, click, release, pressed, disabled, and focus states render correctly.
- Sliders support click and drag and save the chosen value.
- Dropdowns open, select an item, and close.
- A modal blocks clicks behind it.
- Clicking one control cannot activate a second control.
- Pointer location remains aligned after scaling, resolution change, or fullscreen transition.
- Right-click does not accidentally trigger gameplay.
- Keyboard and gamepad can operate the same screen.

Gameplay-specific mouse checks:
- Entering a run captures/hides the cursor only when configured for mouse steering.
- Relative movement steers in the intended direction; inversion and sensitivity settings work.
- The system cursor is not repeatedly warped or stuck at screen center.
- Escape pauses, releases capture, and leaves a visible working cursor.
- Focus loss pauses and releases capture immediately.
- Resume restores steering without reversed axes, huge accumulated deltas, or a frozen cursor.
- Switching to keyboard/gamepad disables mouse-flight deltas without affecting UI clicks.
- Disconnect/reconnect of a controller never leaves movement or boost stuck.

**Release gate:** zero known mouse-hitbox, stuck-cursor, input-context, and pause/resume defects in the required matrix.

## 2. Seed determinism

- Generate the same seed/version/ruleset twice and compare canonical course-data hashes.
- Verify independent cosmetic changes do not change obstacle layouts.
- Verify a new generator version is recorded explicitly and does not masquerade as the old course version.
- Seed-entry parsing must reject empty/invalid input gracefully and normalise equivalent textual forms deterministically.
- A replay/ghost must reject incompatible generator or physics versions clearly.

## 3. Procedural playability

- Run large headless seed batches for every difficulty tier and each ship profile.
- Check tunnel seam continuity, minimum aperture, curvature bounds, and collision/render agreement.
- Validate dynamic hazards over their full relevant timing window.
- Detect unavoidable obstacle intersections and insufficient warning distance.
- Confirm resource placements never block the only required route.
- Confirm a failed candidate regenerates with a finite retry limit and useful diagnostic output.
- Record each failure by seed, generator version, section index, ship, subsystem, and invariant.
- Keep regression seeds for every previously discovered generator bug.

## 4. Performance and streaming

- Measure frame time at maximum allowed speed and highest supported obstacle density.
- Generate and validate sections ahead of the ship; no generator job may stall a render frame.
- Verify worker results from an abandoned run cannot enter a new run.
- Verify GPU resources are created/destroyed on the rendering thread.
- Unloading behind the player cannot break collision, replay, or the current camera.
- Verify bounded memory use during long endless sessions.

## 5. Save and economy tests

- Fresh install creates a valid profile after the first meaningful save.
- Hidden attributes are applied on Windows where supported.
- Restart restores settings, currency, unlocked ships, progress, and records.
- Corrupt primary file recovers from backup.
- Corrupt primary and backup files trigger a recoverable UI path.
- Interrupt the save during each write stage and verify at least one valid profile remains.
- Duplicate-click purchase cannot deduct twice or grant twice.
- Insufficient funds never deduct currency.
- Unlock rewards are persisted before the success message is shown.
- Uninstall does not silently delete player progress.

## 6. AI and collision

- Bots cannot pass through tunnel walls or generated obstacles.
- Bot route choices use the same validated course and respect craft-specific clearance.
- Difficulty changes behavior through navigation quality/risk/reaction parameters rather than illegal movement.
- Bot collisions and player collisions use consistent timing.
- AI does not appear inside the player or a wall after section streaming.

## 7. Camera and gameplay

- FPP/TPP switch works while moving and while using boost.
- Camera switching changes presentation only; it does not alter craft transform, collision, seed, or obstacle phase.
- TPP camera collision prevents clipping through tunnel geometry.
- FPP/TPP both show the upcoming route clearly enough for fair reaction.
- Pausing freezes run simulation and moving hazards according to the chosen pause rule.
- Restarting a seed run reproduces the same underlying course.

## 8. Build and distribution

- Clean Windows x64 build from documented commands.
- Automated tests and procedural validators pass in CI.
- Installer and portable package include all required runtime assets.
- Launch works when started from outside the repository and outside the build directory.
- A clean install can launch, play, save, restart, load, update, and uninstall.
- Version numbers and release artifacts agree.
- No completion claim until the packaged build itself has been tested.
