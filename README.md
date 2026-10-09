# TUNRUN
### A seed-driven procedural 3D tunnel runner

TUNRUN is a planned native Windows 3D flight game built around **unpredictable, procedurally generated tunnels and obstacles**. The player flies through continuously generated curved, twisted, compressed, and expanding environments while navigating shapes and motion that are not known in advance.

This repository is in the **planning and architecture stage**. The documents below define what must be built and how it will be tested before feature implementation begins.

## Core pillars

- **Real procedural generation:** seeded randomness, coherent noise, generated geometry, and combinatorial obstacle construction—not a fixed sequence of preset obstacles.
- **Curving 3D tunnels:** changing centerlines, cross-sections, twists, widths, entrances, and transitions.
- **Fair unpredictability:** generated content is validated for clearance, reachability, warning time, and player movement limits.
- **FPP and TPP:** switch between first-person and third-person cameras without resetting the run.
- **Three input families:** keyboard, mouse, and gamepad operate in gameplay and every UI screen.
- **AI pilots and drones:** bots navigate the same generated world and obey the same collision rules.
- **Persistent progression:** distinct ships, unlocks, records, settings, and two original resource types.
- **Offline-first save:** versioned local save with atomic writes, a backup, recovery handling, and a hidden Windows file/folder attribute.
- **Native Windows release:** C++20, CMake, raylib, reproducible build steps, automated validation, installer and updater planned.

## Game outline

- **Working project name:** TUNRUN
- **Genre:** procedural 3D tunnel runner / arcade flight
- **Primary platform:** Windows x64
- **Proposed stack:** C++20, raylib 5.5, CMake, Windows-native packaging
- **World model:** continuously generated curved tunnel sections from a reproducible seed and generator version
- **Modes planned:** campaign, endless, seed challenge, daily challenge, practice, rival run, and ghost race
- **Currencies:** Aether Shards (common) and Singularity Cores (rare)
- **Persistence:** local profile under `%LOCALAPPDATA%\\Tamasrazim\\TUNRUN\\`

## Planning documents

1. [Game Design](docs/GAME-DESIGN.md) — gameplay, modes, flight, ships, AI, progression, and economy.
2. [Procedural Generation](docs/PROCEDURAL-GENERATION.md) — seed model, noise-shaped geometry, generated obstacles, and fairness validation.
3. [Technical Architecture](docs/TECHNICAL-ARCHITECTURE.md) — engine structure, runtime loop, generation pipeline, and build boundaries.
4. [Input and Save System](docs/INPUT-AND-SAVE.md) — keyboard/mouse/gamepad support, explicit input contexts, hidden saves, and recovery.
5. [QA and Acceptance](docs/QA-AND-ACCEPTANCE.md) — concrete pass/fail checks, especially for mouse interaction and procedural playability.

## Non-negotiable requirements

- No UI control may depend on the gameplay mouse-capture state.
- The mouse cursor must be visible and clickable on menus, hangar, settings, pause, confirmation dialogs, and results.
- Input contexts must change explicitly when gameplay starts, pauses, loses focus, and resumes.
- A seed plus generator version and relevant settings must reproduce the same underlying course.
- Noise may shape geometry, but all generated geometry must remain inside validated limits.
- Procedural content must be generated ahead of the player; section generation must not freeze gameplay.
- Bots must obey world geometry and collision rules rather than teleporting through obstacles.
- Currency transactions and unlocks must save consistently and cannot charge twice.
- A feature is not complete until its acceptance checks pass.

## Current status

**Planning only.** No playable game or installer is claimed to exist yet. Implementation should begin only after this spec and architecture are reviewed, then proceed in small, testable milestones.
