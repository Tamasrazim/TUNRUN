# TUNRUN
### A seed-driven procedural 3D tunnel runner

[![Windows build and tests](https://github.com/Tamasrazim/TUNRUN/actions/workflows/windows-build.yml/badge.svg)](https://github.com/Tamasrazim/TUNRUN/actions/workflows/windows-build.yml)
[![Linux deterministic tests](https://github.com/Tamasrazim/TUNRUN/actions/workflows/linux-tests.yml/badge.svg)](https://github.com/Tamasrazim/TUNRUN/actions/workflows/linux-tests.yml)
[![Repository integrity](https://github.com/Tamasrazim/TUNRUN/actions/workflows/repo-checks.yml/badge.svg)](https://github.com/Tamasrazim/TUNRUN/actions/workflows/repo-checks.yml)
[![Secret scan](https://github.com/Tamasrazim/TUNRUN/actions/workflows/secrets-scan.yml/badge.svg)](https://github.com/Tamasrazim/TUNRUN/actions/workflows/secrets-scan.yml)

![TUNRUN roadmap status — M0 complete; M1–M5 in progress; M6–M9 planned](docs/progression-status.svg)

**Progress snapshot:** [Milestone status and acceptance gaps](docs/PROGRESS.md) · [Full roadmap](docs/ROADMAP.md)

TUNRUN is a native Windows 3D flight game project built around **unpredictably generated tunnels and obstacles**. The intended full game flies through curved, twisted, compressed, and expanding environments.

The repository has moved into native implementation. **M5 persistence and ship economy are in progress**: schema v3 stores career-best score/combo records, retains the accidental-corruption checksum, and tests v1/v2 upgrade paths; settings and progression save locally with backup recovery; the hangar uses separately confirmed Aether Shard and Singularity Core unlocks, with equipping saved as a separate action; and each ship has distinct speed, acceleration, boost-drain and energy-recovery parameters. Seeded tunnel generation, four deterministic aperture-gate families with finite-depth throats, moving procedural mines, and collectible Aether Shards/Singularity Cores now form the gameplay prototype. Each 18-unit-per-side throat shares its center/radius function across rendering, swept collision, route guidance, and camera clearance. Obstacle generator v6 keeps the narrow core for 9 units on each side of the gate plane and gives every gate one of four seeded passage shapes: Single S, Double S, Helical Weave, or Split Wave. Lateral/vertical amplitudes remain bounded at 1.45–1.90 and 0.20–0.35 units. The shared geometry keeps gate-plane alignment and maximum bend range consistent, and the collision corridor follows the same sampled shape. Route guidance follows the current shared throat centreline and uses its sampled slope as lateral/vertical velocity feed-forward, including the trailing half after gate-plane crossing. Lateral movement authority was slightly increased to keep the deeper bend steerable; forward speed targets stay unchanged. Opaque tunnel surfaces, gate details, mines, pickups and wall filaments fade toward a near-black depth fog. Gate validation includes both an all-ship pairwise kinematic screen and a fixed-step route probe that propagates a concrete position/velocity trajectory through consecutive gates using gameplay physics. The probe is a witness trajectory, not an exhaustive proof of every reachable state. Seed Lab displays course, gate, mine, and reward hashes with validation summaries and caches validation by seed/active ship rather than recomputing every frame. Its bounded route graph stratifies the 32-state beam across nine aim offsets and cruise/boost/precision flight policies, interleaving modes before pruning, aims in a course-relative frame using both current and gate centerline samples, uses gameplay's gate-crossing interpolation, applies the live swept moving-mine collision model, uses a unit-tested local avoidance projection that predicts mine phase using forward arrival speed and checks the projected craft path (not only the gate aim), carries elapsed time per trajectory, and reports the best witness's weakest gate clearance plus beam-pruned and mine-hit candidate counts; parent trajectories are prioritized by that weakest gate margin without removing the beam-diversity rule. The ranking uses stable insertion sort on the fixed candidate array, avoiding temporary heap allocation in the noexcept validator. Pickup contact is evaluated in the same course-relative frame as its rendered position. Gate and mine crash screens report the procedural obstacle index and deterministic distance for easier seed-based reproduction. The Windows workflow builds/packages the native app and runs MSVC tests; a Linux/GCC workflow also runs the deterministic and persistence test target. These tests do not claim that the native game application is supported on Linux; TUNRUN is not a finished game.

## Core pillars

- **Real procedural generation:** seeded randomness, coherent noise, generated geometry, and compositional obstacle construction—not a fixed sequence of preset obstacles.
- **Seeded rewards:** Aether Shards render as cyan faceted gems and rarer Singularity Cores as larger warm-metallic 3D pickups; swept crossing checks award their run payout.
- **Obstacle readability:** standard, precision, offset and wide gates keep their gameplay-defined apertures while a separate seeded visual channel selects five support silhouettes: Radial Cage, Segmented Crown, Chevron Brace, Twin Rails, and Split Clamps. The HUD names both the gate type and its support silhouette. Six seed-selected mine shells use distinct Orbital, Prism, Rotor, Cross, Halo Array, and Shard Cluster silhouettes/palettes, while an independent seeded channel gives each mine a lateral-sweep, vertical-sweep, elliptic-orbit or figure-eight trajectory. An in-world segmented warning beacon appears 54 units out, accelerates toward a second urgent ring at 22 units, and follows the rendered mine in the curved course frame. The warning animation is visual-only; mine collision still uses a shared spherical proxy, so the visual variety does not claim six different collision shapes.
- **Skill scoring and records:** clean gate passes earn accuracy bonuses, gate families have different base scores, and consecutive passes raise the in-run combo multiplier. Career-best score and combo persist in profile v3, the Records / Statistics screen displays them, and the crash/results screen distinguishes new personal records from failed saves.
- **Input and pause safety:** W accelerates, S brakes, and A steers left and B steers right (D also works as a right-steer alias); lateral turns smoothly bank the visible ship. Mouse movement looks around using the camera without steering the craft or rolling the view. In first-person, the reticle follows the final camera ray and stays aligned when the view is clipped by a tunnel wall or narrow gate throat. The HUD adds a live left/right/up/down cue toward the next narrow S-bend throat and keeps following the current throat after the gate plane until the spacecraft exits its sleeve. The gamepad remains supported, with the left stick for movement and the right stick for rotation. Arrow keys rotate the ship, and nose direction agrees with flight drift (right/up heading produces right/up movement); Q/E roll, Shift boosts, Ctrl enables precision flight, Space triggers dash, and V changes camera. Mouse-look sensitivity is configurable and saved locally. Crash and Seed Lab seeds can be copied in exact round-trippable hexadecimal form. Destructive pause actions and settings reset ask for confirmation.
- **Refresh-aware frame pacing:** the renderer follows the monitor refresh rate up to 240 FPS, falls back to 144 FPS when the monitor query is invalid, and keeps flight physics on its separate fixed timestep.
- **Curving 3D wormhole tunnel:** changing centerlines, cross-sections, twists and widths, plus five deterministic 24-unit wall-architecture zones: ribbed metal, plasma rails, fractured panels, spiral conduits, and lattice. Zone motifs change panel seams, rib density, rail arrangement, and muted accent palettes while preserving the opaque continuous tunnel skin. Narrow gate bulkheads keep their tapered, twisting inner passage; rendering, swept gate-throat collision, route guidance, and FPP/TPP camera clearance continue to share the same sampled throat center and radius. Near-black depth fog fades distant tunnel surfaces and objects without creating see-through gaps.
- **Fair unpredictability:** geometry bounds, gate clearance and reaction spacing are validated. A ship-specific pairwise kinematic screen and bounded 32-state route graph sample trajectories with cruise, boost, precision and one-shot dash controls using gameplay flight physics. The initial 32 candidates are mode-balanced as well as each later beam. The graph is empirical multi-trajectory coverage, not a formal exhaustive reachability proof. The Seed Lab reports mine-collision counts alongside total collision checks and shows the reason when a bounded search finds no witness. Seed Lab reports the outcomes as WITNESS or NO WITNESS rather than PASS or FAIL: a bounded-search miss means the sampled policies did not find a witness, not that a safe trajectory is impossible. Broader seed coverage and hardware warning-time validation remain unfinished.
- **FPP and TPP:** switch camera presentation without resetting the run. Mouse-look controls camera yaw/pitch independently from ship heading; in TPP it moves the camera eye around the ship rather than rotating the view from a fixed point. Camera eye position and look rays are checked against the generated tunnel and aperture throats. The hangar uses a live 3D turntable preview. All eight ships have solid low-poly hulls, ship-specific filled wings, canopy glass and twin engine details, with different handling parameters.
- **Three input families:** keyboard, mouse, and gamepad operate gameplay and UI navigation; Seed Entry includes keyboard/paste and a gamepad character picker.
- **AI pilots and drones:** bots navigate the same generated world and obey the same collision rules.
- **Persistent progression (in progress):** settings, active ship, seed sequence, run/crash records, best distance, Aether Shards, and Singularity Cores persist locally. Ship purchase/unlock transactions are implemented; campaign progression remains unfinished.
- **Offline-first save:** versioned local save with atomic writes, backup, recovery, and a hidden Windows file/folder attribute.
- **Ship progression:** unlock prices are ten times the previous values; handling parameters are unchanged. Harder procedural courses have stronger bends/twist, tighter minimum tunnel sections, more precision/offset gates, and six mine silhouettes.
- **Native Windows release:** C++20, CMake and raylib are the selected prototype stack; the full game, save system, installer and updater remain unfinished.

## Game outline

- **Project/repository name:** TUNRUN
- **Genre:** procedural 3D tunnel runner / arcade flight
- **Primary platform:** Windows x64
- **Stack:** C++20, CMake, raylib 5.5 (pinned to an immutable upstream commit)
- **Save location:** `%LOCALAPPDATA%\\Tamasrazim\\TUNRUN\\`
- **Modes:** Endless survival, custom-seed runs, and Practice Preview are available. Campaign, daily/seed challenge variants, rival runs, and ghost racing remain planned.

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

**TUNRUN is still a native gameplay prototype, not a finished release.** Profile v3 stores settings, active ship, seed progression, wallets, best distance, career-best score and best combo. Atomic writes, an accidental-corruption checksum, v1/v2 migration, backup recovery, and explicit non-destructive recovery are implemented and covered by automated tests. Campaign checkpoints, daily challenge variants, full hazard-family expansion and streaming, AI, imported production assets, installer and updater remain unfinished. Endless mode enters the procedural survival loop and ends on collision; mode-specific records are not implemented yet. four distinct procedural mine silhouettes and a longer section of the course are now shown at once. Windows CI now stages a portable development ZIP with license notices and a SHA-256 manifest; it is not the final installer. Current source-generated geometry does not justify padding the package—production assets must be real and licensed. Check [GitHub Actions](https://github.com/Tamasrazim/TUNRUN/actions) for build/test status. No finished game is claimed.


### Windows package size
This is still a code-driven prototype and has no production model, texture, or audio packs. A future 300–400 MB Windows package should earn its size through licensed models, materials, animations, music/SFX, and bundled runtime assets—not empty padding, duplicate files, or filler. No package at that size is claimed yet.
