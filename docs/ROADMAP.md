# Implementation Roadmap

This roadmap defines evidence required to advance. A milestone is not complete until its acceptance criteria pass in a real build.

## M0 — Specification baseline
**Status:** baseline documents established.
- Design, architecture, input/save, procedural generation, decisions and QA requirements recorded.
- Main-only workflow documented and the root proprietary license is explicit.
- Security policy, asset provenance policy and GitHub integrity/secret checks added.

## M1 — Application shell and input testbed
**Status: in progress.**

Current slice:
- Native raylib window, responsive screen shell, main menu, settings, credits, hangar preview and mode selection.
- Keyboard, pointer, gamepad D-pad and rate-limited left-stick navigation for menu controls.
- Windows Raw Input relative mouse-look with focus-aware capture, cursor clipping (no recentering/warping), and immediate capture release on focus loss or leaving the run.
- A moving 3D tunnel visual testbed with FPP/TPP camera presentation toggle and an in-run mouse-clickable Pause control.
- Pure C++ tests for edge-triggered button activation, screen-stack navigation, mouse camera offset clamping, and first-person/third-person camera placement safety.
- Windows x64 build/test workflow on pushes to `main`.

Still required before M1 can pass:
- Verify the Windows CI build and fix all compile/test failures. Maintain fullscreen startup, safe FPP/TPP placement, keyboard/gamepad movement and rotation, configurable mouse camera look, and pointer-driven UI outside gameplay.
- Verify UI pointer focus/hitboxes on real Windows hardware at normal and scaled DPI, windowed and fullscreen. Relative mouse deltas must be consumed only during an active focused run when mouse camera is enabled, and must not affect menus or craft position.
- Test pointer hitboxes after resize/fullscreen transitions; make every screen fully operable by mouse, keyboard and gamepad.
- Add integration tests for focus-loss pause, capture/release and nested Settings → Pause behavior.

## M2 — Flight and camera prototype
**Status: in progress.** The prototype now has fixed-step lateral/vertical flight with W accelerate, S brake, A/B lateral steering, independent mouse camera look, boost energy, precision steering, a shared analytic tunnel cross-section, wall collision, collectible seeded rewards, eight ship-specific solid low-poly 3D spacecraft, and a retry screen.
**Deliverable:** controllable craft, FPP and TPP, collision with a manually generated curved test tunnel.
- Both cameras work without changing physics.
- Flight remains stable across render rates.
- Wall, obstacle, pause/resume and controller tests pass; obstacle collision and the manual hardware matrix remain outstanding.

## M3 — Deterministic procedural geometry
**Status: in progress.** Generator v1 now produces seeded centerline offsets, bounded radii and twist, Catmull-Rom joins, a canonical course hash, a bounded-parameter validator, and an in-game Seed Lab. Rendering and wall collision read the same seeded cross-section data.
- Same seed/version/ruleset produces the same canonical course hash.
- Curves, twist and cross-section parameters stay within generator bounds.
- Seed Entry accepts literal hexadecimal or normalized text seeds and persists the chosen root seed before starting. The renderer now has tangent-aligned, twist-aware local frames; shared authoritative physics/collision frames, fixed expected hash fixtures across compilers, and taper/flare transitions remain outstanding. Windows/MSVC and Linux/GCC run the same deterministic and persistence suite; Linux CI verifies the test target, not native game-app support.

## M4 — Generated obstacles and validation
**Status: in progress.** Obstacle generator v2 deterministically generates four aperture-gate families (Standard, Precision, Offset, Wide), applies per-family size/offset constraints, renders them with distinct cues, and hashes obstacle parameters independently from tunnel geometry. The validator checks parameter envelopes, nominal clearance and reaction spacing. Alongside the all-ship pairwise screen, a fixed-step route probe now carries lateral position and velocity across successive gates using the same `updateFlight()` routine and swept gate-plane interpolation as gameplay. Seed Lab caches and displays the selected ship's probe result, including minimum clearance and steps simulated. This proves one deterministic witness trajectory for the generated route; an exhaustive reachable-state graph, broader seed fixtures, and real-hardware warning-distance verification remain outstanding.
- The Seed Lab now runs a bounded 32-state route graph. Candidate selection is stratified across parent trajectories and interleaves nine aim offsets across cruise, boost, precision, and one-shot dash modes before pruning. The initial and subsequent beams contain eight candidates per mode; the omitted aim offset rotates by gate so all 36 policy combinations are sampled over successive gates; target positions account for current and gate centerline samples, gate clearance uses gameplay's shared crossing interpolation, steering candidates receive a unit-tested local avoidance projection using estimated forward arrival time and projected craft position, then swept moving-mine collisions use each candidate's own simulation clock. Mine-pruned counts appear in diagnostics. The Seed Lab labels a found path WITNESS and a missed bounded search NO WITNESS, avoiding a false claim that a search miss proves impossibility. The 32-seed × 8-ship batch verifies deterministic metrics for both outcomes. Parents are ordered by their minimum gate clearance using stable insertion sort on the fixed candidate array, so the bounded beam gives safer paths extra branches without temporary allocation; diagnostics report the best witness's weakest gate margin. Continue broadening seed coverage and compare graph survivors against real hardware trajectories; this remains empirical coverage, not exhaustive reachability proof.
- Add an independent holdout seed set and verify the visual warning distance and collision/render agreement on real hardware.
- A first deterministic moving-mine family and collectible-pickup layer are implemented. Pickup collision shares the rendered course-centerline frame, and live gate scoring tracks accuracy bonuses, clean passes, and combo. Pickup collision now shares the rendered course-centerline frame. Hazard contacts use swept relative 3D checks, gate/mine collision uses the sampled course frame, crash screens expose deterministic obstacle indices, and automated tests run a 32-seed × 8-ship route graph regression batch plus edge-contact and stationary-frame fixtures. A multi-state reachability proof, route-dependent rewards, alternate-route reward rules, other hazard families, and generation streaming remain outstanding.

## M5 — Save, resources, and hangar
**Status: in progress.** Profile v3 persists runtime settings, active ship, root-seed/run sequence, wallet balances, run/crash totals, best distance, career-best score and best combo. Writes include a canonical FNV-1a corruption checksum; valid v1 and v2 profiles migrate to v3 after version-appropriate validation while keeping the old primary as backup. Windows uses a per-user local directory, hidden attributes, bounded JSON, flushed temporary writes, atomic replacement, backup recovery, and a recovery/reset UI that preserves damaged copies. The hangar unlocks and equips ships using Aether Shards or Singularity Cores with rollback if profile persistence fails. Ship handling is driven by one catalogue. Tests cover v1/v2 migration, checksum rejection, recovery, economy transactions, and handling differences.
- Complete campaign progression and unlock/progression rewards.
- Exercise migration/write interruption and real Windows restart/permission behavior across the full acceptance matrix.
- The runtime FNV checksum detects accidental corruption only; it is not tamper protection or cryptographic signing.

## M6 — Gameplay and AI
**Deliverable:** hull, boost, dash, scoring, rewards, rival pilots, environmental drones, ghost recording.
- AI respects the same collision constraints.
- Replay metadata validates seed, generator and physics versions.
- Retrying cannot accept stale generation results from an abandoned run.

## M7 — Progression and content breadth
**Deliverable:** campaign, endless mode, seed challenges, practice and mode-specific records.
- Difficulty tiers are measurable and documented.
- Campaign content is complete enough for a full start-to-finish run.

## M8 — Polish, accessibility, and performance
**Deliverable:** audio, visuals, effects, input remapping, accessibility and profiling.
- Reduced-motion mode disables nonessential camera effects.
- Long sessions have bounded memory and no recurring generation stalls.
- Critical input and save bugs are resolved.

## M9 — Release candidate
**Deliverable:** Windows installer, portable package, verified notices, release metadata and checksums.
- Clean-machine install and uninstall passes.
- Full acceptance matrix passes.
- Release binaries and checksums match the published release.

## Latest implementation update — 2026-10-10

- Camera targeting follows mouse-look yaw/pitch independently from the craft's heading. The FPP camera position is clamped within its own tunnel cross-section; the TPP camera base stays behind the course tangent and is clamped to rear-section clearance. Manual runtime camera/occlusion acceptance remains separate from automated tests.
- Four deterministic cosmetic mine families (Orbital, Prism, Rotor, Cross) have distinct silhouette/color treatments and named HUD cues. Cosmetics use a separate seed channel and do not mutate hazard movement or the collision/hash definition.
- Automated tests cover look-target direction/clamping, invalid numeric inputs, deterministic mine family selection and coverage across 512 generated hazards.
- The latest verified Windows build/test, repository-integrity, and secret-scan workflows remain available through the live GitHub Actions results. Manual Windows input/camera acceptance is still separate.

## Priority rule
Fix crashes, input lockups, data loss, impossible generated courses and corrupted progression before adding more obstacle families or cosmetics.
