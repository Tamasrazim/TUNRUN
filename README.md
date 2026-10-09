# TUNRUN
### A seed-driven procedural 3D tunnel runner

TUNRUN is a native Windows 3D flight game project built around **unpredictably generated tunnels and obstacles**. The intended full game flies through curved, twisted, compressed, and expanding environments.

The repository has moved into native implementation. **M3 geometry and M4 obstacle work are in progress**: seeded smooth tunnel cross-sections, a canonical course hash, parameter validation, an in-game Seed Lab, deterministic aperture gates, and gate-plane collision checks. The 120 Hz flight prototype also has forward motion, boost energy, precision steering, wall collision, and retry controls. Windows CI compiles, runs unit tests, and publishes a temporary development artifact. This is an early development build—not a complete game.

## Core pillars

- **Real procedural generation:** seeded randomness, coherent noise, generated geometry, and compositional obstacle construction—not a fixed sequence of preset obstacles.
- **Curving 3D tunnels:** changing centerlines, cross-sections, twists, widths, entrances, and transitions.
- **Fair unpredictability:** generated content is validated for clearance, reachability, warning time, and player movement limits.
- **FPP and TPP:** switch between first-person and third-person cameras without resetting the run.
- **Three input families:** keyboard, mouse, and gamepad operate gameplay and every UI screen.
- **AI pilots and drones:** bots navigate the same generated world and obey the same collision rules.
- **Persistent progression:** distinct ships, unlocks, records, settings, Aether Shards, and Singularity Cores.
- **Offline-first save:** versioned local save with atomic writes, backup, recovery, and a hidden Windows file/folder attribute.
- **Native Windows release:** C++20, CMake and raylib are the selected prototype stack; the full game, save system, installer and updater remain unfinished.

## Game outline

- **Project/repository name:** TUNRUN
- **Genre:** procedural 3D tunnel runner / arcade flight
- **Primary platform:** Windows x64
- **Stack:** C++20, CMake, raylib 5.5 (pinned to an immutable upstream commit)
- **Save location:** `%LOCALAPPDATA%\\Tamasrazim\\TUNRUN\\`
- **Modes planned:** campaign, endless, seed challenge, daily challenge, practice, rival run, and ghost race

## Documentation map

### Design
- [Game Design](docs/GAME-DESIGN.md)
- [Flight and Camera](docs/FLIGHT-AND-CAMERA.md)
- [UI Specification](docs/UI-SPECIFICATION.md)
- [Ships and Economy](docs/ECONOMY-AND-SHIPS.md)
- [Art and Audio](docs/ART-AND-AUDIO.md)

### Engineering
- [Procedural Generation](docs/PROCEDURAL-GENERATION.md)
- [Technical Architecture](docs/TECHNICAL-ARCHITECTURE.md)
- [Bot AI](docs/BOT-AI.md)
- [Input and Save](docs/INPUT-AND-SAVE.md)
- [Save Schema](docs/SAVE-SCHEMA.md)
- [Build and Release](docs/BUILD-AND-RELEASE.md)
- [Security and Privacy](docs/SECURITY-AND-PRIVACY.md)
- [Third-Party and Licenses](docs/THIRD-PARTY-AND-LICENSES.md)
- [Third-Party Notices](THIRD-PARTY-NOTICES.md)

### Project management and quality
- [Decision Log](docs/DECISIONS.md)
- [Roadmap](docs/ROADMAP.md)
- [QA and Acceptance](docs/QA-AND-ACCEPTANCE.md)

## Main-only repository policy

All changes are committed directly to `main`. The project does **not** use other branches or pull requests. Pushes to `main` run repository integrity, secret scanning, and the Windows build/test workflow. Direct-to-main does not mean skipping checks.

## Current status

**M3 geometry and M4 obstacle work are in progress.** Deterministic centerline/cross-section sampling, gate generation, gate collision and the Seed Lab are implemented as an initial prototype. Tangent-aligned local frames, varied/moving hazards, reachability checks, save persistence, AI, economy, assets, installer and updater remain unfinished. Check [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions) for build/test status. No finished game is claimed.
