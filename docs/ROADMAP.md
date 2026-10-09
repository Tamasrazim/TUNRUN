# Implementation Roadmap

This roadmap defines evidence required to advance. It is not a claim that any milestone has already been implemented.

## M0 — Specification baseline
**Deliverable:** approved design/architecture documents, explicit decision log, controls, save schema, procedural requirements, and QA gates.

Exit criteria:
- Open decisions have owners and are resolved before affected implementation.
- Readme links point to existing planning files.
- No feature is described as implemented unless demonstrated in a build.

## M1 — Application shell and input testbed
**Deliverable:** native window, state machine, UI framework, action-based input service, settings and focus-loss handling.

Exit criteria:
- Main menu, settings and pause can be operated with keyboard, mouse and gamepad.
- Hit testing works at tested DPI/window sizes.
- Pause/focus loss releases gameplay input; no repeated cursor warping.
- Automated tests cover edge-triggered button actions and screen-stack behavior.

## M2 — Flight and camera prototype
**Deliverable:** controllable craft, FPP and TPP, collision with a manually generated curved test tunnel.

Exit criteria:
- Both cameras work without changing physics.
- Flight remains stable across target render rates.
- Wall, obstacle, pause/resume and controller tests pass.
- A small playable test build is available to inspect handling.

## M3 — Deterministic procedural geometry
**Deliverable:** seeded centerlines, stable local frames, configurable cross-sections, seamless section joins, test seed viewer.

Exit criteria:
- Same seed/version/ruleset produces the same canonical course hash.
- Curves, twist, taper, flare and cross-section variations remain inside declared bounds.
- A generated section can be visualised independently for debugging.

## M4 — Generated obstacles and validation
**Deliverable:** compositional procedural structures, moving geometry, clearance validator, dynamic reachability checks.

Exit criteria:
- The generator creates variations from parameters rather than a fixed obstacle order.
- Invalid candidates fail with reproducible diagnostics.
- Batch tests find no known unavoidable collision in the accepted regression set.
- Generation happens ahead of the player without stalling a frame.

## M5 — Save, resources, and hangar
**Deliverable:** hidden local profile, backup/recovery, currency transactions, ship catalog and selection.

Exit criteria:
- Purchases and progression persist across restarts.
- Corrupt primary profile recovers from backup.
- Duplicate activation cannot duplicate an unlock or charge twice.
- Mouse, keyboard and controller can all navigate and use the hangar.

## M6 — Gameplay and AI
**Deliverable:** hull, boost, dash, scoring, rewards, rival pilots, environmental drones, ghost recording.

Exit criteria:
- AI respects the same collision/flight constraints.
- Ghost metadata validates seed, generator and physics versions.
- Run result and rewards are saved reliably.
- Player can retry without stale async generation results contaminating the new run.

## M7 — Progression and content breadth
**Deliverable:** first campaign stage set, endless mode, seed challenges, practice and mode-specific records.

Exit criteria:
- Difficulty tiers are measurable and documented.
- Campaign objectives are complete enough for a full start-to-finish run.
- Replay and economy balance playtests are documented.

## M8 — Polish, accessibility, and performance
**Deliverable:** audio, visuals, effects, settings, input remapping, accessibility and performance profiling.

Exit criteria:
- High-speed hazards remain readable in FPP and TPP.
- Reduced-motion mode disables nonessential camera effects.
- Long sessions show bounded memory and no recurring generation stalls.
- All critical input and save bugs are resolved.

## M9 — Release candidate
**Deliverable:** Windows installer, portable package, updater if retained, documentation and release page.

Exit criteria:
- Clean-machine install and uninstall.
- Full acceptance matrix passes.
- Reproducible regression-seed report included with the release candidate.
- All build commands and limitations are verified against the release source.

## Priority rule

Fix P0 input lockups, broken saves, impossible generated courses, data loss, and crashes before adding new obstacle families or cosmetic content.
