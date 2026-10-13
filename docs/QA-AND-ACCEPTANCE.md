# QA and Acceptance Plan

A feature is not complete because its screen exists. It is complete only when its acceptance criteria pass in a real build.

## 1. Input and mouse regression matrix

Test on Windows at normal DPI and scaled DPI, windowed, borderless, fullscreen, after resizing, and after Alt+Tab.

For each screen (Main Menu, Hangar, Records / Statistics, Controls, Mode Select, Seed Entry, Settings, Pause, Results, confirmation dialogs):
- Every button is mouse-clickable at its visible position.
- Records / Statistics opens from the Main Menu and displays persisted best distance, score, combo, run/crash totals, wallet balances, unlocked ship count and active ship; Back returns to the main menu with keyboard, pointer and gamepad while preserving the main-menu selection.
- Campaign is clearly marked as in development and cannot accidentally launch a run; Endless Survival starts the available procedural survival loop, while Custom Seed Run and Practice Preview perform their documented actions. Back exits the Mode Select screen exactly once.
- Crash/results view shows final distance, active seed, elapsed in-run time, course hash, score, rewards and the applicable gate/mine contact index without overlapping the action menu.
- Standard, Precision, Offset and Wide gate families have distinct ring/support details while preserving the same circular collision aperture used by physics.
- Copy Seed in Crash and Seed Lab writes the exact lowercase fixed-width `0x`-prefixed 64-bit seed to the OS clipboard; pasting it into Seed Entry starts the exact same course, including leading zeroes. Generating another Seed Lab seed clears the previous copied notice. Retry Same Seed remains the first/default crash action.
- Seed Lab's five actions remain visible and clickable at the minimum supported window height without overlap.
- Controls opens from both Main Menu and Pause, lists actual keyboard/mouse/gamepad bindings, and Back returns to the correct parent screen without unpausing the run.
- While Controls, Settings or run-confirmation is layered above Pause, procedural hazard time remains frozen; returning to the run does not advance the mines through the pause interval. Unit tests cover the shared run-clock gating for Pause, nested screens, Settings and Crash.
- Pause → Restart Same Seed asks before discarding the current run, retains the exact course seed after confirmation, and starts with clean score, dash, and pickup state.
- Pause → Return to Main Menu asks before discarding the current run; Cancel returns to Pause without changing the run.
- On the minimum-height window, the first menu button does not overlap the currency row.
- At the minimum supported window size, all nine Main Menu actions remain visible and clickable without overlap.
- Every launch starts fullscreen even if the last session exited windowed; the in-session fullscreen toggle remains functional.
- The renderer selects the reported monitor refresh rate up to 240 FPS, falls back to 144 for invalid reports, and leaves fixed-step physics independent of the render cap.
- The Hangar's live 3D model matches the previewed ship, stays clear of navigation/action hitboxes at minimum window width, and updates immediately after changing the preview ship.
- A locked ship opens an explicit unlock confirmation showing its exact price and current resource balance; Cancel leaves balances and unlock state unchanged, and Confirm persists the unlock before reporting success.
- Unlocking a ship does not equip it. Equipping is a separate saved action, and a failed save rolls back the corresponding wallet/unlock or active-ship change.
- Hover, click, release, pressed, disabled, and focus states render correctly.
- Sliders support click and drag and save the chosen value.
- During a focused gameplay run with mouse-look enabled, relative mouse input rotates the camera only and never steers or moves the spacecraft. Mouse capture releases on focus loss, pause, or leaving gameplay. A/B/D and the gamepad left stick steer, W/S control forward speed, and keyboard arrows or the gamepad right stick control yaw/pitch rotation; Q/E roll remains finite under sustained input.
- The tunnel has filled interior panels plus wireframe ribs; ships and reward pickups use shaded low-poly 3D geometry, and all six mine visual families have distinct 3D silhouettes. Mine collision remains the documented shared spherical proxy, independent of cosmetic shell details.
- Settings navigation remains fully visible at the minimum window size; all seven rows, including Back, remain clickable after resize/fullscreen changes.
- Entering or retrying a run while Space or gamepad A is held does not trigger dash until the player releases and presses it again.
- Reset Options first asks for confirmation. Cancel leaves settings untouched; confirming restores fullscreen, FPS, and reduced-motion defaults without changing wallet balances, ship unlocks, or run history.
- Reduced Motion suppresses nonessential animated dash streaks while preserving dash physics, collision timing and the textual DASH ACTIVE / recharge indicator.
- Dropdowns open, select an item, and close.
- A modal blocks clicks behind it.
- Clicking one control cannot activate a second control.
- Pointer location remains aligned after scaling, resolution change, or fullscreen transition.
- The game is rendered to a fixed 1280×720 logical canvas (16:9). Test 16:9, 16:10, ultrawide, windowed and fullscreen displays: the image must remain undistorted, letterbox bars must stay outside the game canvas, and pointer targets must remain aligned inside the scaled viewport.
- Tunnel-wall and gate-throat collision checks report their first contact along the movement segment, not just a yes/no result. If one frame crosses both boundaries, the earlier impact determines the crash cause. Results must stop at that contact point and time, retain the impacted gate index for a throat crash, and not include distance travelled afterward. Include a generated narrow radius pinch and a side impact inside a throat; repeat at low and high render rates to check for tunnelling.
- Gate rims and the inner throat must be visibly readable before their collision surfaces reach the ship. On a crash, the result screen must distinguish a throat-wall impact from a collision at the gate plane; the two events must not share a misleading explanation.
- Right-click does not accidentally trigger gameplay.
- Keyboard and gamepad can operate the same screen.

Gameplay-specific mouse checks:
- Entering a run captures relative mouse input only when mouse-look is enabled; on Windows the cursor is hidden and clipped to the current client area without recentering warps; the clipping rectangle refreshes after window movement or resizing.
- Relative mouse movement rotates the camera only and never changes craft steering or position. The logarithmic sensitivity slider supports pointer click/drag, bounded left/right keyboard/controller adjustments, and the mouse-look enable/disable toggle; pointer interaction must not require gameplay capture.
- In settled FPP, the reticle is centred on the final camera ray and stays aligned when that ray is shortened by an outer wall or gate throat. It is not a steering target and does not disappear when keyboard/gamepad controls are used. In TPP, mouse-look orbits the camera eye around the ship while keeping the ship as the focus.
- The system cursor is not repeatedly warped or stuck at screen center.
- Escape pauses, releases capture, and leaves a visible working cursor.
- Focus loss pauses and releases capture immediately.
- Resume restores camera look without a stuck cursor or a sudden jump from stale relative deltas.
- Keyboard/gamepad steering and rotation can be used alongside mouse camera look; input must not unexpectedly disable mouse-look or leak relative deltas into UI pointer hit-testing.
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

- Same seed and generator version produce the same canonical hash; a changed seed produces a different course hash. Seed Lab validation is cached by seed and active ship, so reopening the same configuration reuses the result while either key changing triggers a recomputation.
- Generated centerline, radius, twist, gate aperture, and gate offsets remain inside declared bounds.
- Gate distances are strictly increasing, retain minimum reaction spacing, and leave nominal tunnel clearance for the craft.
- Obstacle generator v14 keeps throat lateral amplitude within 1.10–1.45 units, vertical amplitude within 0.14–0.28, and sampled slope within the corresponding 0.28/0.16 ceilings. Verify the first gate is 34–38 units ahead, gate planes are 54 units apart, the 18-unit-per-side sleeves leave an 18-unit recovery stretch, and the first throat collision zone does not begin before 16 units; mine planes must be at least 20 units from neighboring gate planes. Re-run the 512-gate suite for every ship; also perform a real build playtest because a bounded route witness cannot establish human handling fairness.
- Swept gate-plane tests detect obstacle contact between fixed simulation steps while a craft in the opening clears the gate.
- Run large headless seed batches for every difficulty tier and each ship profile. The automated suite checks 512 gates for all eight ship profiles and 64-gate batches across 24 deterministic derived seeds.
- Run both the fixed-step single-trajectory route probe and the bounded state-propagating route graph across every ship profile and a deterministic multi-seed subset. Both must use gameplay flight physics, crossing interpolation and conservative tunnel clearance. The graph branches viable gate-crossing states into multiple target policies and reports beam width, discarded candidates, gates with multiple passing states and minimum aperture clearance. It is bounded multi-trajectory coverage, not a formal proof of every physically reachable state.
- Check tunnel seam continuity, minimum aperture, stronger left/right bends, visible narrowing/expansion, variable twist, and collision/render agreement.
- Verify reward hashes and pickup order are seed-deterministic, Aether Shard values remain 4–8, every eighth pickup is a Singularity Core, and pickup collision happens only on a forward plane crossing. Pickups render as filled faceted 3D objects with distinct shard/core materials.
- Verify gate collision and scoring consume one computed course-relative crossing point per crossing; outside-aperture points collide and earn nothing, invalid points fail closed, and combo multipliers, clean-pass thresholds, reset behaviour and saturating score arithmetic are deterministic.
- A run that sets a new career-best score or combo announces exactly which record improved; tied or lower results do not show a new-record banner. A failed profile write takes priority over the new-record banner and explicitly warns that this run's results may not be saved.
- Verify pickup render positions and collision offsets use the same sampled course-relative frame as the tunnel while it curves/twists; include a pickup-centered trajectory through a curved section, misses outside the collection radius, backwards movement, and non-finite collision inputs.
- Verify moving-mine generator v5 and hashes are seed-deterministic, motion is repeatable for equal run-clock times, faster oscillation stays within the 0.95–1.80 frequency bounds, motion stays within the declared spatial envelope, and every mine plane stays at least 20 course units from the nearest gate plane so its collision envelope does not overlap a gate throat.
- Mine HUD bearing matches the render-frame transform and advances to the next mine immediately after a plane crossing; offsets are deterministic for equal seed, run clock and player pose, while non-finite inputs disable the cue safely.
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
- Keep accumulated course distance in double precision and verify fixed-step travel still advances at large distances (at least 1,048,576 course units).

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
- Camera switching changes presentation only; it does not alter craft position, collision rules, seed, or obstacle phase. TPP's camera stays within the tunnel on the rear centerline; the rear tube geometry must enclose it at all times.
- TPP camera placement is measured from the rear section's centerline, clamps to the tunnel radius with wall clearance, and stays finite for invalid inputs.
- The gate HUD shows CLEAR/PREPARE/APPROACH/IMMINENT based on the next gate's remaining distance and actual forward speed. Tests cover the 4.0 s, 2.5 s, and 1.0 s thresholds, acceleration/dash timing differences, and invalid values.
- Ordinary scenery and the world-space throat marker fade to the exact background by 17 course units; positive-infinite look-ahead distances must not reveal scenery. Gate rims and opaque bulkheads must remain legible through their separate 16-to-64-unit fade without revealing tunnel geometry through the aperture. Mine beacons retain their separate warning envelope. Verify that the gate appears early enough at normal speed and during boost/dash, then check for sudden marker pops and missed approach cues. The estimated 46% reduction in tunnel-panel triangle submissions is not a substitute for frame-time profiling.
- FPP/TPP both show the upcoming route clearly enough for fair reaction.
- Pausing freezes run simulation and moving hazards according to the chosen pause rule.
- Restarting a seed run reproduces the same underlying course.

## 8. Security and privacy

- Malformed, oversized, truncated, checksum-mismatched, and schema-incompatible save files fail safely without a crash or unbounded allocation.
- Valid v1 and v2 fixtures upgrade to v3; the v1 primary remains as backup, the v2 checksum is verified against the original field set, and the newly written v3 checksum validates after reload.
- Save, replay, seed and imported-asset paths cannot escape their designated directories; archive extraction rejects absolute paths and `..` traversal.
- JSON numbers, arrays, strings, nesting depth, entity counts and replay durations have explicit limits.
- Corrupted primary and backup saves produce a recoverable error and never silently reset progression.
- Profile reads and resets inspect path entries without following symlinks; dangling profile links enter recovery, reset preserves them, and saves never write through linked temporary or backup paths.
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
