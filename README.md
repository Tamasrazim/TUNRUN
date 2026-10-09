# TUNRUN
### A seed-driven procedural 3D tunnel runner

TUNRUN is a planned native Windows 3D flight game built around **unpredictable, procedurally generated tunnels and obstacles**. The player flies through continuously generated curved, twisted, compressed, and expanding environments while navigating geometry and motion that are not known in advance.

This repository is in the **planning and architecture stage**. These documents describe the intended game, technical boundaries, and pass/fail tests. They are not a claim that a playable build exists.

## Core pillars

- **Real procedural generation:** seeded randomness, coherent noise, generated geometry, and compositional obstacle construction—not a fixed sequence of preset obstacles.
- **Curving 3D tunnels:** changing centerlines, cross-sections, twists, widths, entrances, and transitions.
- **Fair unpredictability:** generated content is validated for clearance, reachability, warning time, and player movement limits.
- **FPP and TPP:** switch between first-person and third-person cameras without resetting the run.
- **Three input families:** keyboard, mouse, and gamepad operate gameplay and every UI screen.
- **AI pilots and drones:** bots navigate the same generated world and obey the same collision rules.
- **Persistent progression:** distinct ships, unlocks, records, settings, Aether Shards, and Singularity Cores.
- **Offline-first save:** versioned local save with atomic writes, backup, recovery, and a hidden Windows file/folder attribute.
- **Native Windows release:** C++20, CMake and raylib are the proposed stack; build, installer and updater remain to be implemented and verified.

## Game outline

- **Project/repository name:** TUNRUN
- **Genre:** procedural 3D tunnel runner / arcade flight
- **Primary platform:** Windows x64
- **Proposed stack:** C++20, raylib 5.5, CMake, Windows-native packaging
- **World model:** continuous curved tunnel sections from a reproducible seed and generator version
- **Modes planned:** campaign, endless, seed challenge, daily challenge, practice, rival run, and ghost race
- **Currencies:** Aether Shards (common) and Singularity Cores (rare)
- **Save location:** `%LOCALAPPDATA%\\Tamasrazim\\TUNRUN\\`

## Documentation map

### Design
1. [Game Design](docs/GAME-DESIGN.md) — loop, modes, ships, AI, progression, economy.
2. [Flight and Camera](docs/FLIGHT-AND-CAMERA.md) — flight model, FPP/TPP, collision, boost, tuning targets.
3. [UI Specification](docs/UI-SPECIFICATION.md) — screen inventory, navigation, prompts, visual/accessibility rules.
4. [Ships and Economy](docs/ECONOMY-AND-SHIPS.md) — proposed ship catalog, unlocks, currencies, transaction rules.
5. [Art and Audio](docs/ART-AND-AUDIO.md) — visual language, asset pipeline, effects, sound requirements.

### Engineering
6. [Procedural Generation](docs/PROCEDURAL-GENERATION.md) — seed model, noise geometry, generated structures, fairness validation.
7. [Technical Architecture](docs/TECHNICAL-ARCHITECTURE.md) — modules, runtime loop, threading and build boundaries.
8. [Bot AI](docs/BOT-AI.md) — rival navigation, drones, difficulty profiles and ghost recordings.
9. [Input and Save](docs/INPUT-AND-SAVE.md) — device actions, mouse-capture state rules, save behavior.
10. [Save Schema](docs/SAVE-SCHEMA.md) — profile fields, validation, safe writes, migration.
11. [Build and Release](docs/BUILD-AND-RELEASE.md) — planned layout, main-branch checks, packaging and release checklist.
12. [Security and Privacy](docs/SECURITY-AND-PRIVACY.md) — threat boundaries, save/input safety, update integrity and privacy.
13. [Third-Party and Licenses](docs/THIRD-PARTY-AND-LICENSES.md) — dependency and asset provenance requirements.

### Project management and quality
14. [Decision Log](docs/DECISIONS.md) — confirmed requirements versus proposals still needing validation.
15. [Roadmap](docs/ROADMAP.md) — milestones and exit criteria.
16. [QA and Acceptance](docs/QA-AND-ACCEPTANCE.md) — input regressions, determinism, fairness, security, saves, AI and release tests.

## Non-negotiable requirements

- No UI control may depend on gameplay mouse capture.
- The cursor remains visible and clickable on menus, hangar, settings, pause, dialogs and results.
- Input contexts change explicitly on run start, pause, focus loss and resume.
- Same seed + generator version + ruleset/settings reproduces the same underlying course.
- Noise may shape geometry, but generated geometry remains inside validated physical limits.
- Sections are generated ahead of the player; accepted content is validated before use.
- Bots obey world geometry and collision rules rather than teleporting through obstacles.
- Currency transactions and unlocks save consistently and cannot charge/grant twice.
- A feature is not complete just because its screen exists; acceptance checks must pass.

## Security and repository policy

- [Security Policy](SECURITY.md) explains private vulnerability reporting.
- [Contributing](CONTRIBUTING.md) records the project workflow: commits go directly to `main`; there are no extra branches or pull requests.
- [Security and Privacy](docs/SECURITY-AND-PRIVACY.md) defines implementation safeguards. Main-only GitHub Actions validate documentation and scan Git history for hardcoded secrets; game-code analysis will be added when source code exists.
- [Third-Party and Licenses](docs/THIRD-PARTY-AND-LICENSES.md) records asset/dependency review rules. A project-wide reuse license has not yet been selected.

## Current status

**Documentation baseline in progress; game implementation has not started.** Core design, generation, architecture, input/save, AI, economy, UI, release planning, decisions and acceptance documents are now specified. Remaining design choices are marked as proposals in [Decision Log](docs/DECISIONS.md) and must be validated by prototypes. No playable game or installer is claimed to exist.
