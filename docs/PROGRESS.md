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
| M7 — Progression and content | In progress | Endless survival entry is available; campaign, challenge variants, richer Practice and mode-specific records remain. |
| M8 — Polish and accessibility | Planned | Production art/audio, remapping, accessibility and long-session profiling. |
| M9 — Release candidate | Planned | Installer/portable release validation, clean-machine tests, checksums and acceptance matrix. |

The segmented bar is intentionally **not** labelled with an overall completion percentage: the five active milestones cover very different amounts of work. A roadmap milestone remains active until its acceptance criteria are explicitly satisfied.

## Latest implementation slice

- FPP's eye now sits forward and slightly above the ship origin inside the canopy. A centered tunnel-frame reticle returns in stable FPP. Camera roll blends out across the FPP-to-TPP transition, and the spacecraft fades in only after the chase camera clears its hull. If a tight sleeve forces the camera close to the ship, model visibility also fades by actual camera-to-ship distance. The HUD reports actual units/second after throttle, boost, precision and dash, plus estimated time to the next mine based on that same live speed.
- FPP/TPP switching now blends the camera eye and focus over a short frame-rate-independent transition. At a stable mode endpoint, the renderer performs only that view's ray-clearance scan; extra candidate and blended-ray scans run only during the transition. Both candidate poses are clamped to the active tunnel/throat, the intermediate blended eye is reprojected inside its local cross-section, and the final look ray is checked again so the transition cannot cut a corner through an S-bend wall. The ship model follows the blend in and out rather than appearing/disappearing at the mode toggle.
- TPP mouse orbit keeps the spacecraft as its actual focus instead of a fixed point ahead along the course. If the direct view ray intersects a curved wall or narrow gate sleeve, the camera moves inward along the last clear segment rather than turning the view onto the obstruction. The ray clearance tapers from the eye's larger wall margin to the ship's collision-hull margin, so a valid ship position near the aperture edge does not force the camera to ignore it. FPP remains at the pilot viewpoint and clips its look ray at obstructions.
- Camera regression tests cover the focus distance used by FPP versus TPP, verify that throat-obstructed reverse rays request a camera pull-in, confirm the ship's collision-hull margin is more permissive than the camera-eye margin, and check that the FPP reticle stays aligned to the final camera ray even when that ray is shortened by a wall.
- Endless survival is now a selectable mode and the main Play action enters it directly; the live HUD names the active mode. Custom Seed and Practice launch paths set their own run identity, while Campaign remains explicitly unfinished.
- Camera aim follows mouse-look angles independently from ship yaw/pitch. In TPP, mouse-look moves the camera eye around the spacecraft while the focus stays on the ship. The camera moves closer when a wall or throat blocks the view; ray-clearance margins taper from the camera eye to the ship's collision-hull margin. FPP stays at the cockpit with a mouse-directed, tunnel-clamped look target.
- W accelerates above cruise, S brakes even when boost or precision is held, blocks new dash activations, and settles back toward cruise when released. An already-triggered dash keeps its short committed burst; regression assertions cover these priorities. Positive yaw/pitch now drive the ship in the same right/up direction the rendered nose points, with dedicated regression tests; lateral steering banks the ship smoothly into turns and returns it toward level when released, without rolling the tunnel camera.
- Obstacle generator v9 retains four actual throat waveform families: Single S, Double S, Helical Weave, and Split Wave. Split Wave balances its single/double-harmonic lateral bend to retain visible displacement through both halves of the sleeve while using its third harmonic vertically. Tests sample both sides of the minimum-aperture core across 512 deterministic gates. An endpoint easing envelope preserves each waveform's interior and makes its bend slope reach zero at the sleeve-to-tunnel join. Each gate retains independent bend amplitudes (1.45–1.90 lateral and 0.20–0.35 vertical) and the same 18-unit-per-side throat with a 9-unit narrow core. The blended harmonics preserve the existing unit envelope, return to the original center at the gate plane and sleeve ends, and feed the same shared sampler for rendering, swept collision, route guidance, and camera clearance. Family and amplitudes are included in the obstacle hash. Automated coverage samples all four families over 512 deterministic gates; manual in-game handling checks remain separate. The route graph follows the current shared throat centreline, derives its lateral/vertical slope from the same sampler, and uses that as velocity feed-forward. It keeps tracking the previous throat after gate-plane crossing instead of abruptly switching to the next gate target. Lateral movement authority was slightly increased for the deeper curve without changing forward speed. A sampled camera-ray limit now checks both the outer tunnel wall and the shared narrow throat surfaces, preventing the view centre from cutting through either between otherwise-safe endpoints; regression coverage includes rays that would cross a solid sleeve. Stronger near-field depth fog hides later sections beyond each opening. The exact gate plane still uses the shared interpolated crossing test.
- Opaque tunnel surfaces remain continuous but are fogged toward the near-black background with depth. Distant tunnel ribs, gate sleeves and rings, moving mines, pickups, and animated wormhole filaments lose contrast instead of showing crisp detail far beyond a narrow opening.
- The Hangar uses explicit keyboard focus: confirming Back cannot accidentally purchase/equip the selected ship. Purchase/equip is a separate focused action, and the shop layout now adapts to narrower windows.
- All eight spacecraft now use filled, ship-specific wing planforms with a defined canopy and twin engine bells instead of only thin outline silhouettes.
- Narrow gate frames have opaque bulkheads around their real apertures, preventing the rest of the next section from showing through. Six animated wall filaments follow the generated centerline and twist for a more wormhole-like continuous tunnel.
- Controls map W to accelerate, S to brake, A to steer left and B to steer right (D remains a right-steer alias), and mouse movement to camera look only. The keyboard mapping is now a pure tested helper; tests cover W/S, A/B, the D alias, opposed-key neutralisation, rotation and roll. The HUD computes a future throat-centre cue from the shared sleeve sampler, calls for pitch up/down where vertical alignment is needed, keeps tracking the current throat after its gate plane, switches to the next throat only after exiting the current one, and renders a small 3D target ring at that same sampled centre in both FPP and TPP. When a gamepad is connected, keyboard and right-stick rotation now combine instead of the stick's resting axis overriding the keyboard.
- Camera target follows the spacecraft's yaw/pitch using a target point clamped inside the sampled tunnel frame. Longitudinal target distance follows the heading, including controlled look-back.
- Six seed-selected mine presentation families are implemented: Orbital, Prism, Rotor, Cross, Halo Array, and Shard Cluster. Family names are reflected in the in-run hazard cue; these are cosmetic and do not alter the mine physics hash.
- Camera targeting and visual-family coverage have automated unit assertions.
- The Seed Lab route diagnostics use a bounded, fixed-size state graph. Its 32-state beam samples nine aim offsets in cruise, boost, precision, and one-shot dash modes. The initial beam and each subsequent 32-state beam are interleaved into eight candidates per mode; the omitted aim offset rotates each gate so all policies get sampled over the route, stratifies candidates across surviving trajectories, and aims in a course-relative frame using current and gate centerline samples, shares gameplay's gate-crossing interpolation, applies the live swept moving-mine collision model, uses a unit-tested local avoidance projection that estimates arrival time from forward speed and checks projected craft position as well as the gate aim. Per-candidate clocks and mine-pruned counts remain visible, and surviving parents are prioritized by their weakest gate clearance using stable insertion sort on the fixed candidate array, avoiding temporary allocation. Automated tests cover every ship on a 12-gate baseline and replay deterministic diagnostics for a 32-seed × 8-ship batch. The Seed Lab also reports mine hits against collision checks and shows the search-miss reason when no witness is found. Because this graph is bounded and heuristic, Seed Lab uses WITNESS and NO WITNESS labels instead of PASS/FAIL; no witness is a search miss, not proof that no safe route exists. This remains empirical coverage, not exhaustive reachability proof.
- Repository integrity, secret scanning and Windows build/unit-test workflows run on pushes to `main`. Their state is available through live badges and [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions).

## Update rule

After each meaningful implementation chunk, update this snapshot, the root README's progress section, `docs/ROADMAP.md`, and the main-site project page if user-visible capability or status changes. Never mark a milestone complete merely because its UI or documentation exists.


### Moving hazard motion profiles

Generator version 3 now assigns every mine one of four reproducible trajectories: lateral sweep, vertical sweep, elliptic orbit, or figure-eight. This motion channel is seeded independently from the four cosmetic shell silhouettes. The HUD shows both layers, motion profiles are included in the canonical hazard hash, and tests cover deterministic family selection and movement bounds. Collision still uses the shared spherical proxy; richer multi-part hazard choreography remains future work.


### Procedural tunnel-wall architecture

The renderer now selects one of five deterministic decorative wall motifs every 24 course units: Ribbed Metal, Plasma Rails, Fractured Panels, Spiral Conduits, or Lattice. Motifs vary panel seams, rib density, active rail lanes, and subdued palette through a rendering-only seed channel. The opaque tunnel remains continuous and continues to use the existing collision sampler; no decorative motif creates a gameplay obstacle. Coverage and deterministic repeatability are unit-tested across 512 sections.


### Procedural gate architecture variety

A visual-only gate generator now chooses between Radial Cage, Segmented Crown, Chevron Brace, Twin Rails, and Split Clamps through a seed channel independent of gate difficulty and collision data. The renderer applies each as a different annular support pattern while preserving the existing safe opening. The flight HUD reports the gameplay gate kind together with the selected structure. Automated tests check repeatability, family coverage and the minimum visual clearance; manual in-game readability still needs a separate review.


### Mine proximity warning pass

Moving mines now carry a segmented in-world warning ring in their own sampled tunnel frame. The ring starts at 54 course units, pulses faster as the craft closes in, and adds a second ring plus an urgent HUD label at 22 units. The warning uses the same animated center as the mine mesh but remains purely visual; the mine's spherical collision proxy and procedural hash are unchanged. Tests cover threshold edges, pulse bounds and determinism.


### Expanded mine silhouette library

Two new procedural mine meshes extend the existing four: Halo Array uses three intersecting hoops around a core, and Shard Cluster uses six faceted crystal fins. Palette and silhouette are chosen on the existing visual-only seed channel; movement and hitbox generation are unchanged. Automated coverage checks all six shells over 512 deterministic hazard indices. In-game readability and frame-time review still need to be performed.


- The in-run HUD now displays the selected throat waveform family (S-Bend, Double S, Helical Weave, or Split Wave) beside the course seed and steering cue. This makes the generated passage topology inspectable during flight instead of leaving the family label available only to diagnostics/tests.
