# TUNRUN
### A seed-driven procedural 3D tunnel runner

TUNRUN is a native Windows 3D flight game project built around **unpredictably generated tunnels and obstacles**. The intended full game flies through curved, twisted, compressed, and expanding environments.

The repository has moved into native implementation. **M5 persistence and ship economy are in progress**: schema v3 stores career-best score/combo records, retains the accidental-corruption checksum, and tests v1/v2 upgrade paths; settings and progression save locally with backup recovery; the hangar uses Aether Shards and Singularity Cores to unlock and equip ships; and each ship has distinct speed, acceleration, boost-drain and energy-recovery parameters. Seeded tunnel generation, four deterministic aperture-gate families, moving procedural mines, and collectible Aether Shards/Singularity Cores now form the gameplay prototype. Gate validation includes both an all-ship pairwise kinematic screen and a fixed-step route probe that propagates a concrete position/velocity trajectory through consecutive gates using gameplay physics. The probe is a witness trajectory, not an exhaustive proof of every reachable state. Seed Lab displays course, gate, mine, and reward hashes with validation summaries; pickup contact is evaluated in the same course-relative frame as its rendered position. Gate and mine crash screens report the procedural obstacle index and deterministic distance for easier seed-based reproduction. The Windows CI build and unit tests are the source of truth; this is not a finished game.

## Core pillars

- **Real procedural generation:** seeded randomness, coherent noise, generated geometry, and compositional obstacle construction—not a fixed sequence of preset obstacles.
- **Seeded rewards:** Aether Shards and rarer Singularity Core pickups appear in the tunnel, use swept crossing checks, and contribute to the run payout.
- **Moving hazards:** seed-derived mines oscillate over time, appear in the 3D tunnel with a HUD distance cue, and can end a run on contact.
- **Skill scoring and records:** clean gate passes earn accuracy bonuses, gate families have different base scores, and consecutive passes raise the in-run combo multiplier. Career-best score and combo persist in profile v3, the Records / Statistics screen displays them, and the crash/results screen calls out new personal records.
- **Energy dash:** Space or gamepad A triggers a bounded forward burst that consumes boost energy and then recharges on a cooldown.
- **Curving 3D tunnels:** changing centerlines, cross-sections, twists, widths, entrances, and transitions.
- **Fair unpredictability:** geometry bounds, gate clearance and reaction spacing are validated. A ship-specific pairwise kinematic reachability screen is now included; a full propagated-state proof and warning-time validation remain unfinished.
- **FPP and TPP:** switch between first-person and third-person cameras without resetting the run. All eight ships have distinct procedural wireframe silhouettes in TPP and a matching 2D hangar preview, alongside separate handling parameters.
- **Three input families:** keyboard, mouse, and gamepad operate gameplay and UI navigation; Seed Entry includes keyboard/paste and a gamepad character picker.
- **AI pilots and drones:** bots navigate the same generated world and obey the same collision rules.
- **Persistent progression (in progress):** settings, active ship, seed sequence, run/crash records, best distance, Aether Shards, and Singularity Cores persist locally. Ship purchase/unlock transactions are implemented; campaign progression remains unfinished.
- **Offline-first save:** versioned local save with atomic writes, backup, recovery, and a hidden Windows file/folder attribute.
- **Configurable mouse steering:** sensitivity is adjustable in the Settings screen with keyboard and gamepad controls and persists in the local profile; Reset Options preserves progression.
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
- [Procedural Rewards](docs/PROCEDURAL-REWARDS.md)
- [Procedural Moving Hazards](docs/PROCEDURAL-HAZARDS.md)
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

**M3/M4 geometry and obstacle work, and M5 persistence, are in progress.** Profile v3 stores settings, active ship, seed progression, wallets, best distance, career-best score and best combo. Atomic writes, an accidental-corruption checksum, v1/v2 migration, backup recovery, and explicit non-destructive recovery are implemented and covered by automated tests. Campaign checkpoints, cross-platform hardware acceptance, dynamic hazards, AI, assets, installer and updater remain unfinished. Check [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions) for build/test status. No finished game is claimed.
