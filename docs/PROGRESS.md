# TUNRUN progress snapshot

This file is the current snapshot used by the README progress bar and project-page roadmap status. It reports roadmap states, not a guessed percentage of features or lines of code.

## Milestone state

| Milestone | State | What remains before it can close |
|---|---|---|
| M0 — Specification baseline | Complete | Maintain the documentation and governance baseline. |
| M1 — Application shell and input | In progress | Real Windows hardware checks for mouse capture, DPI/resize hitboxes, focus loss and nested input contexts. |
| M2 — Flight and camera | In progress | Verify camera/flight behavior on hardware; continue handling and collision/visibility checks. |
| M3 — Procedural geometry | In progress | Broader centerline/frame continuity, cross-section transitions and fixed expected cross-platform hash fixtures. The same deterministic and persistence tests now run under both Windows/MSVC and Linux/GCC; literal cross-compiler hash fixtures remain the next verification step. |
| M4 — Obstacles and validation | In progress | Multi-state route reachability, more seed regressions, and hardware-verified warning times. |
| M5 — Save, resources and hangar | In progress | Real Windows migration/write-interruption tests and fuller progression rewards. |
| M6 — Gameplay and AI | Planned | Hull/damage, rival pilots, drones and compatible replay/ghost pipeline. |
| M7 — Progression and content | Planned | Campaign, Endless, challenges, Practice and mode-specific records. |
| M8 — Polish and accessibility | Planned | Production art/audio, remapping, accessibility and long-session profiling. |
| M9 — Release candidate | Planned | Installer/portable release validation, clean-machine tests, checksums and acceptance matrix. |

The segmented bar is intentionally **not** labelled with an overall completion percentage: the five active milestones cover very different amounts of work. A roadmap milestone remains active until its acceptance criteria are explicitly satisfied.

## Latest implementation slice

- Camera aim now follows mouse-look angles independently from ship yaw/pitch; the third-person camera base stays behind the course tangent through sharp turns rather than swinging behind the ship's nose. FPP camera position is also clamped to its own local cross-section so a narrow rear section cannot clip the camera through the wall.
- W accelerates above cruise, S brakes even when boost or precision is held, blocks new dash activations, and settles back toward cruise when released. An already-triggered dash keeps its short committed burst; regression assertions cover these priorities.
- Narrow apertures now get an opaque, tapered inner passage whose radius and offset match the gate's collision opening. The sleeve twists gently along its depth, improving the wormhole feel and breaking up the long straight sightline through a flat ring.
- The Hangar uses explicit keyboard focus: confirming Back cannot accidentally purchase/equip the selected ship. Purchase/equip is a separate focused action, and the shop layout now adapts to narrower windows.
- All eight spacecraft now use filled, ship-specific wing planforms with a defined canopy and twin engine bells instead of only thin outline silhouettes.
- Narrow gate frames have opaque bulkheads around their real apertures, preventing the rest of the next section from showing through. Six animated wall filaments follow the generated centerline and twist for a more wormhole-like continuous tunnel.
- Controls map W to accelerate, S to brake, A to steer left and B to steer right (D remains a right-steer alias), and mouse movement to camera look only. Normal cruise remains at the previous speed; new deterministic tests cover acceleration and braking.
- Camera target follows the spacecraft's yaw/pitch using a target point clamped inside the sampled tunnel frame. Longitudinal target distance follows the heading, including controlled look-back.
- Four seed-selected mine presentation families are implemented: Orbital, Prism, Rotor and Cross. Family names are reflected in the in-run hazard cue; these are cosmetic and do not alter the mine physics hash.
- Camera targeting and visual-family coverage have automated unit assertions.
- The Seed Lab route diagnostics use a bounded, fixed-size state graph. Its 32-state beam samples nine aim offsets in cruise, boost, precision, and one-shot dash modes. The initial beam and each subsequent 32-state beam are interleaved into eight candidates per mode; the omitted aim offset rotates each gate so all policies get sampled over the route, stratifies candidates across surviving trajectories, and aims in a course-relative frame using current and gate centerline samples, shares gameplay's gate-crossing interpolation, applies the live swept moving-mine collision model, uses a unit-tested local avoidance projection that estimates arrival time from forward speed and checks projected craft position as well as the gate aim. Per-candidate clocks and mine-pruned counts remain visible, and surviving parents are prioritized by their weakest gate clearance using stable insertion sort on the fixed candidate array, avoiding temporary allocation. Automated tests cover every ship on a 12-gate baseline and replay deterministic diagnostics for a 32-seed × 8-ship batch. The Seed Lab also reports mine hits against collision checks and shows the search-miss reason when no witness is found. Because this graph is bounded and heuristic, Seed Lab uses WITNESS and NO WITNESS labels instead of PASS/FAIL; no witness is a search miss, not proof that no safe route exists. This remains empirical coverage, not exhaustive reachability proof.
- Repository integrity, secret scanning and Windows build/unit-test workflows run on pushes to `main`. Their state is available through live badges and [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions).

## Update rule

After each meaningful implementation chunk, update this snapshot, the root README's progress section, `docs/ROADMAP.md`, and the main-site project page if user-visible capability or status changes. Never mark a milestone complete merely because its UI or documentation exists.
