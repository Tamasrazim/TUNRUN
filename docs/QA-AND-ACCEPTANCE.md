# QA and Acceptance Plan

A feature is not complete because its screen exists. It is complete only when its acceptance criteria pass in a real build.

## 1. Input and mouse regression matrix

Test on Windows at normal DPI and scaled DPI, windowed, borderless, fullscreen, after resizing, and after Alt+Tab.

For each screen (Main Menu, Hangar, Records / Statistics, Controls, Mode Select, Seed Entry, Settings, Pause, Results, confirmation dialogs):
- Every button is mouse-clickable at its visible position.
- Records / Statistics opens from the Main Menu and displays persisted best distance, score, combo, run/crash totals, wallet balances, unlocked ship count and active ship; Back returns to the main menu with keyboard, pointer and gamepad while preserving the main-menu selection.
- Crash/results view shows final distance, active seed, elapsed in-run time, course hash, score, rewards and the applicable gate/mine contact index without overlapping the action menu.
- Copy Seed in Crash and Seed Lab writes the exact lowercase fixed-width `0x`-prefixed 64-bit seed to the OS clipboard; pasting it into Seed Entry starts the exact same course, including leading zeroes. Generating another Seed Lab seed clears the previous copied notice. Retry Same Seed remains the first/default crash action.
- Seed Lab's five actions remain visible and clickable at the minimum supported window height without overlap.
- Controls opens from both Main Menu and Pause, lists actual keyboard/mouse/gamepad bindings, and Back returns to the correct parent screen without unpausing the run.
- While Controls, Settings or run-confirmation is layered above Pause, procedural hazard time remains frozen; returning to the run does not advance the mines through the pause interval. Unit tests cover the shared run-clock gating for Pause, nested screens, Settings and Crash.
- Pause → Restart Same Seed asks before discarding the current run, retains the exact course seed after confirmation, and starts with clean score, dash, and pickup state.
- Pause → Return to Main Menu asks before discarding the current run; Cancel returns to Pause without changing the run.
- On the minimum-height window, the first menu button does not overlap the currency row.
- At the minimum supported window size, all nine Main Menu actions remain visible and clickable without overlap.
- The Hangar's 2D silhouette preview matches the currently selected ship, stays clear of navigation/action hitboxes, and updates immediately after changing the preview ship.
- Hover, click, release, pressed, disabled, and focus states render correctly.
- Sliders support click and drag and save the chosen value.
- Mouse sensitivity renders a visible logarithmic track and thumb; click/drag adjusts it, while keyboard arrows and gamepad D-pad/left stick also work. It clamps to the supported range, writes once when a pointer drag ends (including focus loss), and persists across restart. The default sensitivity maps back to itself, and slider positions increase monotonically.
- Settings navigation remains fully visible at the minimum window size; all seven rows, including Back, remain clickable after resize/fullscreen changes.
- Entering or retrying a run while Space or gamepad A is held does not trigger dash until the player releases and presses it again.
- Reset Options first asks for confirmation. Cancel leaves settings untouched; confirming restores fullscreen, FPS, reduced-motion, mouse steering and mouse sensitivity defaults without changing wallet balances, ship unlocks, or run history.
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
- Right-trigger boost and left-trigger precision controls activate above the neutral axis value, ignore non-finite axis readings, and release as soon as the trigger returns to neutral.

**Release gate:** zero known mouse-hitbox, stuck-cursor, input-context, and pause/resume defects in the required matrix.

## 2. Seed determinism

- Generate the same seed/version/ruleset twice and compare canonical course-data hashes.
- Verify independent cosmetic changes do not change obstacle layouts.
- Verify a new generator version is recorded explicitly and does not masquerade as the old course version.
- Seed-entry parsing must reject empty/invalid input gracefully, reject text over 64 characters and zero as a persisted root seed, normalize equivalent text consistently, and parse literal hexadecimal seeds without changing their numeric value.
- Verify Seed Entry persists the root seed before transitioning into a run, restores previous in-memory profile values on save failure, and is reachable from both Seed Lab and Seed Challenge.
- Verify keyboard typing/paste, mouse controls, and the gamepad character picker can enter a seed and apply/cancel without activating a background control.
- A replay/ghost must reject incompatible generator or physics versions clearly.

## 3. Procedural playability

- Same seed and generator version produce the same canonical hash; a changed seed produces a different course hash.
- Generated centerline, radius, twist, gate aperture, and gate offsets remain inside declared bounds.
- Gate distances are strictly increasing, retain minimum reaction spacing, and leave nominal tunnel clearance for the craft.
- Swept gate-plane tests detect obstacle contact between fixed simulation steps while a craft in the opening clears the gate.
- Run large headless seed batches for every difficulty tier and each ship profile. The automated suite checks 512 gates for all eight ship profiles and 64-gate batches across 24 deterministic derived seeds.
- Run the fixed-step route probe across every ship profile and a deterministic multi-seed subset. It must carry position and lateral velocity between gate crossings, use gameplay flight physics and crossing interpolation, remain inside the conservative tunnel clearance, and report non-negative aperture clearance. This proves only the tested controller trajectory, not exhaustive reachable-state coverage.
- Check tunnel seam continuity, minimum aperture, curvature bounds, and collision/render agreement.
- Verify reward hashes and pickup order are seed-deterministic, Aether Shard values remain 4–8, every eighth pickup is a Singularity Core, and pickup collision happens only on a forward plane crossing.
- Verify gate collision and scoring consume one computed course-relative crossing point per crossing; outside-aperture points collide and earn nothing, invalid points fail closed, and combo multipliers, clean-pass thresholds, reset behaviour and saturating score arithmetic are deterministic.
- A run that sets a new career-best score or combo announces exactly which record improved; tied or lower results do not show a new-record banner.
- Verify pickup render positions and collision offsets use the same sampled course-relative frame as the tunnel while it curves/twists; include a pickup-centered trajectory through a curved section, misses outside the collection radius, backwards movement, and non-finite collision inputs.
- Verify moving-mine generation and hashes are seed-deterministic, motion is repeatable for equal run-clock times, motion stays within the declared envelope, and hazard planes maintain the minimum spacing from gate reaction windows.
- Verify swept gate and mine collision transform ship coordinates into the obstacle's sampled course frame; include curved-centreline offsets, near/far sphere-edge contact, near misses, stationary-distance render frames, reverse travel, and non-finite inputs.
- Run multi-seed hazard validation (currently 24 derived seeds × 128 hazards) and gameplay simulations for all eight ships; confirm warning distance gives enough time to evade. Parameter bounds alone are not proof of avoidability.
- Validate any additional dynamic hazard families over their full relevant timing windows.
- Detect unavoidable obstacle intersections and insufficient warning distance.
- Confirm optional pickup placement never narrows or blocks the required route; do not make pickups mandatory until route-reachability validation includes them.
- Ensure moving mines stay separate from gate reaction windows and log seed, mine index, ship, run-clock time, and collision coordinates for any unavoidable encounter.
- Confirm a failed candidate regenerates with a finite retry limit and useful diagnostic output.
- Record each failure by seed, generator version, section index, ship, subsystem, and invariant; gate/mine crash screens should expose the deterministic obstacle index and distance.
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
- Restart restores settings, currency, unlocked ships, progress, best distance, career-best score, and best combo.
- Valid v1 and v2 profile fixtures migrate to v3; altered v2 payloads are rejected by the original-version checksum check, and the legacy primary/backup is preserved during migration.
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
- Dash activation requires a press edge, deducts energy once, ends after the configured duration, respects cooldown, and cannot auto-repeat while held.
- Camera switching changes presentation only; it does not alter craft transform, collision, seed, or obstacle phase.
- TPP camera collision prevents clipping through tunnel geometry.
- FPP/TPP both show the upcoming route clearly enough for fair reaction.
- Pausing freezes run simulation and moving hazards according to the chosen pause rule.
- Restarting a seed run reproduces the same underlying course.

## 8. Security and privacy

- Malformed, oversized, truncated, checksum-mismatched, and schema-incompatible save files fail safely without a crash or unbounded allocation.
- The v1 fixture upgrades to v2, retains the pre-migration primary as backup, and writes a v2 checksum that validates after reload.
- Save, replay, seed and imported-asset paths cannot escape their designated directories; archive extraction rejects absolute paths and `..` traversal.
- JSON numbers, arrays, strings, nesting depth, entity counts and replay durations have explicit limits.
- Corrupted primary and backup saves produce a recoverable error and never silently reset progression.
- No credentials, signing keys, personal profile data, or local build paths are present in tracked files or release packages.
- No update, asset-download, or telemetry code runs without explicit documented approval and a security review.
- Any later updater verifies authenticated release metadata and artifact signatures before replacing files; invalid, stale, or rolled-back packages are rejected.
- Dependencies, fonts, music, sound effects, textures, models and other redistributables have recorded licenses and provenance.
- Security checks run on pushes to `main`; review the actual workflow result rather than treating a configured workflow as proof of safety.

## 9. Build and distribution

- Clean Windows x64 build from documented commands.
- Automated tests and procedural validators pass in CI.
- Installer and portable package include all required runtime assets.
- Launch works when started from outside the repository and outside the build directory.
- A clean install can launch, play, save, restart, load, update, and uninstall.
- Version numbers and release artifacts agree.
- No completion claim until the packaged build itself has been tested.

- Verify an active dash displays animated cyan tunnel streaks and a `DASH ACTIVE` status, then transitions to recharge/energy status when the burst ends.
