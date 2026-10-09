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
- Windows Raw Input relative mouse flight steering with focus-aware capture, cursor clipping (no recentering/warping), and immediate capture release on focus loss or leaving the run.
- A moving 3D tunnel visual testbed with FPP/TPP camera presentation toggle and an in-run mouse-clickable Pause control.
- Pure C++ tests for edge-triggered button activation, screen-stack navigation, and relative mouse steering/clamping.
- Windows x64 build/test workflow on pushes to `main`.

Still required before M1 can pass:
- Verify the Windows CI build and fix all compile/test failures.
- Verify raw mouse steering and focus/capture transitions on real Windows hardware at normal and scaled DPI, windowed and fullscreen.
- Test pointer hitboxes after resize/fullscreen transitions; make every screen fully operable by mouse, keyboard and gamepad.
- Add integration tests for focus-loss pause, capture/release and nested Settings → Pause behavior.

## M2 — Flight and camera prototype
**Status: in progress.** The prototype now has fixed-step lateral/vertical flight, boost energy, precision steering, a shared analytic tunnel cross-section, wall collision, and a retry screen.
**Deliverable:** controllable craft, FPP and TPP, collision with a manually generated curved test tunnel.
- Both cameras work without changing physics.
- Flight remains stable across render rates.
- Wall, obstacle, pause/resume and controller tests pass; obstacle collision and the manual hardware matrix remain outstanding.

## M3 — Deterministic procedural geometry
**Status: in progress.** Generator v1 now produces seeded centerline offsets, bounded radii and twist, Catmull-Rom joins, a canonical course hash, a bounded-parameter validator, and an in-game Seed Lab. Rendering and wall collision read the same seeded cross-section data.
- Same seed/version/ruleset produces the same canonical course hash.
- Curves, twist and cross-section parameters stay within generator bounds.
- Stable tangent-aligned local frames, taper/flare transitions, persistent user-entered seeds, and cross-platform hash fixtures remain outstanding.

## M4 — Generated obstacles and validation
**Status: in progress.** Generator v1 now places seed-derived aperture gates at deterministic distances, draws them in the course, sweeps craft movement through their collision planes, and validates aperture clearance and gate spacing. The Seed Lab reports course and obstacle validation.
- Invalid candidates fail with reproducible diagnostics.
- Batch tests cover seam continuity, obstacle clearance and reaction distances.
- Moving hazards, broader reachability validation, and generation streaming remain outstanding.

## M5 — Save, resources, and hangar
**Deliverable:** hidden local profile, backup/recovery, currency transactions, ship catalog and selection.
- Purchases/progression persist across restarts.
- Corrupt primary save recovers from backup; failed saves are reported.
- Duplicate activation cannot duplicate an unlock or charge twice.

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

## Priority rule
Fix crashes, input lockups, data loss, impossible generated courses and corrupted progression before adding more obstacle families or cosmetics.
