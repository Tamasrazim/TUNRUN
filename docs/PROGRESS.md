# TUNRUN progress snapshot

This file is the current snapshot used by the README progress bar and project-page roadmap status. It reports roadmap states, not a guessed percentage of features or lines of code.

## Milestone state

| Milestone | State | What remains before it can close |
|---|---|---|
| M0 — Specification baseline | Complete | Maintain the documentation and governance baseline. |
| M1 — Application shell and input | In progress | Real Windows hardware checks for mouse capture, DPI/resize hitboxes, focus loss and nested input contexts. |
| M2 — Flight and camera | In progress | Verify camera/flight behavior on hardware; continue handling and collision/visibility checks. |
| M3 — Procedural geometry | In progress | Broader centerline/frame continuity, cross-section transitions and cross-platform determinism fixtures. |
| M4 — Obstacles and validation | In progress | Multi-state route reachability, more seed regressions, and hardware-verified warning times. |
| M5 — Save, resources and hangar | In progress | Real Windows migration/write-interruption tests and fuller progression rewards. |
| M6 — Gameplay and AI | Planned | Hull/damage, rival pilots, drones and compatible replay/ghost pipeline. |
| M7 — Progression and content | Planned | Campaign, Endless, challenges, Practice and mode-specific records. |
| M8 — Polish and accessibility | Planned | Production art/audio, remapping, accessibility and long-session profiling. |
| M9 — Release candidate | Planned | Installer/portable release validation, clean-machine tests, checksums and acceptance matrix. |

The segmented bar is intentionally **not** labelled with an overall completion percentage: the five active milestones cover very different amounts of work. A roadmap milestone remains active until its acceptance criteria are explicitly satisfied.

## Latest implementation slice

- Camera target follows the spacecraft's yaw/pitch using a target point clamped inside the sampled tunnel frame. Longitudinal target distance follows the heading, including controlled look-back.
- Four seed-selected mine presentation families are implemented: Orbital, Prism, Rotor and Cross. Family names are reflected in the in-run hazard cue; these are cosmetic and do not alter the mine physics hash.
- Camera targeting and visual-family coverage have automated unit assertions.
- The Seed Lab route diagnostics use a bounded, fixed-size state graph. Its 32-state beam samples nine aim offsets in cruise, boost, and precision modes, interleaves those modes across the capped beam before pruning, stratifies candidates across surviving trajectories, and aims in a course-relative frame using current and gate centerline samples, shares gameplay's gate-crossing interpolation, applies the live swept moving-mine collision model, uses a unit-tested local avoidance projection that estimates arrival time from forward speed and checks projected craft position as well as the gate aim. Per-candidate clocks and mine-pruned counts remain visible, and surviving parents are prioritized by their weakest gate clearance using stable insertion sort on the fixed candidate array, avoiding temporary allocation. Automated tests cover every ship on a 12-gate baseline and replay deterministic diagnostics for a 32-seed × 8-ship batch. The Seed Lab also reports mine hits against collision checks and shows the search-miss reason when no witness is found. Because this graph is bounded and heuristic, Seed Lab uses WITNESS and NO WITNESS labels instead of PASS/FAIL; no witness is a search miss, not proof that no safe route exists. This remains empirical coverage, not exhaustive reachability proof.
- Repository integrity, secret scanning and Windows build/unit-test workflows run on pushes to `main`. Their state is available through live badges and [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions).

## Update rule

After each meaningful implementation chunk, update this snapshot, the root README's progress section, `docs/ROADMAP.md`, and the main-site project page if user-visible capability or status changes. Never mark a milestone complete merely because its UI or documentation exists.
