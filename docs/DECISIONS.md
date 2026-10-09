# Decision Log

This file distinguishes **confirmed requirements** from **proposals that still require a prototype or owner decision**.

## Confirmed requirements

| ID | Decision | Reason |
|---|---|---|
| D-001 | The game is named TUNRUN for the repository/project at this stage. | Repository identity is established; final in-game branding may be revisited. |
| D-002 | The world uses procedural generation, seeded randomness, and noise-shaped geometry. | The central game concept is unpredictability and reproducibility, not shuffled fixed obstacles. |
| D-003 | Tunnel geometry must include curves and may twist, widen, compress, and change cross-section. | The game must not be restricted to straight tunnels. |
| D-004 | Obstacles and entrances must be generated from parameterised geometry and behaviour. | Avoid a fixed repeating obstacle sequence. |
| D-005 | FPP and TPP are both required and can be switched during a run. | Dual perspective is a core requirement. |
| D-006 | Keyboard, mouse, and gamepad must operate gameplay and all UI screens. | No device is second-class. |
| D-007 | Mouse focus/capture bugs are a release-blocking class of defect. UI must retain reliable pointer interaction. | The mouse/UI regression must not recur. |
| D-008 | AI pilots and environmental bots must obey the same physical world/collision rules. | Fairness and readable behavior. |
| D-009 | The game stores a hidden local Windows save with backup/recovery. | Offline-first persistence and continuity. |
| D-010 | The hangar includes distinct ships and the economy uses Aether Shards and Singularity Cores. | Progression and earned unlocks. |
| D-011 | The first release is single-player/offline-first; online multiplayer is deferred. | Keeps initial architecture achievable. |
| D-012 | Procedural sections must be generated ahead of the player and validated before use. | Prevent stalls and impossible accepted sections. |

## Proposals requiring prototype validation

| ID | Proposal | How to resolve |
|---|---|---|
| P-001 | C++20 + raylib 5.5 + CMake. | Build a minimal renderer/input testbed and assess native raw mouse input, camera, controller support and packaging. |
| P-002 | 100-stage campaign plus endless mode. | Validate content breadth and stage goals once a playable prototype exists. |
| P-003 | Eight named ship archetypes with stat trade-offs. | Prototype two ships first; measure how handling differences interact with generated-course fairness. |
| P-004 | Default cruise/boost speed targets in the flight spec. | Tune after course length, warning distances, and steering response are measurable. |
| P-005 | AI pilots, drones and ghost replay in the first major release. | Implement after procedural navigation data and player flight are stable. |
| P-006 | JSON profile with SHA-256 integrity metadata. | Choose and test the serialization/canonicalisation library before freezing the schema. |
| P-007 | Windows installer, portable ZIP, and updater. | Validate clean build and update behavior before committing to the exact updater design. |
| P-008 | Seed text format and public daily challenge rules. | Define normalization, version identity, and whether daily runs are strictly local or later online-backed. |

## Change policy

- Update this log whenever a confirmed requirement changes.
- A proposal is not a commitment until tested or explicitly promoted to a decision.
- If a decision changes seed generation, physics, save schema, or replay compatibility, increment the relevant version and document migration/compatibility behavior.
- Do not silently change confirmed input behavior or save semantics to simplify implementation.
