# TUNRUN — Game Design

## High concept

TUNRUN is a fast, native 3D tunnel-flight game. The player pilots a spacecraft through a continuously generated environment whose centerline can curve, twist, rise, fall, widen, compress, and change cross-section. Tunnel entrances and obstacles are constructed procedurally, so the player cannot rely on memorising a fixed obstacle sequence.

The core fantasy is **high-speed discovery under pressure**: read a strange shape ahead, understand the available space, and fly through it.

## Design pillars

1. **Unpredictable but learnable.** The next shape is not known in advance, but hazards communicate their movement and the generated course must be physically survivable.
2. **The world is generated, not shuffled.** Geometry, entrances, structure clusters, and obstacle motion emerge from seeded parameters, noise fields, and compositional rules—not a short list of complete prebuilt obstacles.
3. **Flight first.** Camera, acceleration, steering, banking, and collision response should feel coherent in FPP and TPP.
4. **Player choice matters.** Ships have distinct flight trade-offs, alternate routes create risk/reward, and the player can improve through skill.
5. **Input never fights the player.** Keyboard, mouse, and gamepad must operate the full UI and gameplay. In particular, gameplay mouse capture must never leak into menus.
6. **Progress survives.** A hidden local profile stores settings, ships, currency, records, and progression with backup/recovery.

## Main run loop

1. Choose a ship and game mode.
2. Start with a random seed or enter a known seed.
3. The generator builds and validates upcoming tunnel sections ahead of the spacecraft.
4. Fly through curved geometry, generated entrances, structures, moving hazards, and rewards.
5. Earn distance score, precision score, Aether Shards, and challenge rewards.
6. Reach checkpoints or continue until the craft is destroyed or a run objective is completed.
7. Save the result and return to the results screen, then retry, change ship, or share the seed.

The forward motion is automatic by default. Steering controls the craft's position and orientation relative to the local tunnel frame. Optional speed/brake assists may be settings, but the game is not a conventional lane runner.

## Camera modes

### FPP — first-person

- Camera sits at the pilot/cockpit viewpoint and follows craft orientation.
- Tunnel geometry and hazard warnings must remain readable at high speed.
- Cockpit detail is restrained so it does not obscure the route.
- Optional camera shake and motion effects can be reduced or disabled.

### TPP — third-person

- Smooth chase camera shows the full ship and nearby tunnel geometry.
- Camera collision and look-ahead avoid clipping through tunnel walls.
- Curves and twists adjust camera follow without abrupt snapping.
- Distance and FOV are configurable within safe limits.

Pressing the camera-switch input blends between cameras without resetting flight, obstacle timing, seed, or momentum. A camera change must not grant invulnerability or change collision geometry.

## World and procedural space

The course is composed of streamed spatial sections, not a fixed path authored from a sequence of named obstacle types. Each section is generated from a seed-derived parameter set. Adjacent sections must meet position and tangent constraints so transitions feel continuous even when their geometry differs dramatically.

Possible generated properties include:

- centerline curvature and compound bends;
- vertical movement, banking, and helical twist;
- circular, elliptical, asymmetric, and noise-perturbed cross-sections;
- entrances with varying apertures, rims, ribs, layers, and offset geometry;
- generated structures and object clusters;
- moving, rotating, extending, retracting, or phase-shifting components;
- optional routes, reward pockets, environmental effects, and visibility changes.

These are parameter dimensions for generation, not a mandatory sequence. The generator may combine them in unexpected ways, provided the result remains readable and survivable.

## Obstacles and fairness

Obstacle geometry must be generated from compositions and parameters. A generator may combine primitives and deformation fields to create a new structure; it must not simply choose from a tiny hand-built catalogue and call that procedural.

The generator must validate:
- the craft can physically fit through at least one route;
- the player has sufficient distance/time to perceive and react;
- moving components do not close every route simultaneously;
- transitions do not place a hazard directly inside the craft;
- difficulty is compatible with the current progression tier and flight limits.

A warning is part of the obstacle design. Use silhouette, motion, lighting, and spatial placement to communicate the threat rather than relying only on tiny text.

## AI

AI entities inhabit the same course and obey the same collision rules as the player.

- **Rival pilots** compete, choose routes, and make skill-appropriate corrections.
- **Environmental drones** patrol, escort, observe, or cross the flight path.
- **Pursuit entities** may feature in special events but must not use impossible movement.
- **Ghosts** replay saved player trajectories as non-physical competition.

AI difficulty changes prediction horizon, reaction delay, route risk appetite, and consistency—not hidden collision immunity or teleportation. AI must be able to fail, recover when possible, and respect generated clearance.

## Ships and hangar

Ships need distinct silhouettes and handling profiles, not just different paint.

| Ship | Intended identity | Trade-off direction |
|---|---|---|
| Driftwing | Starter / balanced | Predictable handling and balanced durability |
| Wraith | Agile interceptor | Fast direction changes, less hull margin |
| Bulwark | Heavy frame | More hull margin, slower manoeuvring |
| Manta | Stable glider | Smooth response, wider collision profile |
| Comet | Racer | Strong acceleration, demanding precision |
| Spectre | Energy specialist | Efficient boost, lighter protection |
| Vortex | Precision craft | Responsive control, high skill ceiling |
| Obsidian | Advanced prototype | Earned specialist configuration |

These are initial design targets; final numbers must come from playtesting. Every ship must have a real 3D model and collision volume that matches its visual silhouette. The hangar shows a rotatable preview, handling stats, unlock conditions, currency price, and the selected ship.

Performance differences must be trade-offs rather than a universal best ship. The course validator must account for the selected ship's actual collision radius and movement capabilities.

## Resources and economy

- **Aether Shards** — common resource earned through flight pickups, distance milestones, campaign rewards, and challenges.
- **Singularity Cores** — rare resource earned from advanced objectives, special encounters, achievements, and high-difficulty milestones.

The initial game is earned-through-play only; no real-money purchase system is planned. Purchases must be atomic and idempotent: validate unlock conditions and balance, apply one transaction, save, and only then confirm success to the UI. Repeated clicks or a crash must never charge twice or duplicate rewards.

Cosmetics should be separated from handling stats. The game should clearly display whether an unlock changes performance or appearance.

## Game modes

- **Campaign:** structured objectives across proposed stages, with each stage still procedurally generated from its own seed and difficulty constraints.
- **Endless:** survive as far as possible; course generation continues as the player advances.
- **Seed Challenge:** enter/share a seed to reproduce the underlying course with a specified generator version and ruleset.
- **Daily Run:** one common seed/ruleset for the day.
- **Practice:** reduced speed and optional section restart for learning difficult generated passages.
- **Rival Run:** race against AI pilots on the same course.
- **Ghost Race:** compete against stored personal or supplied run recordings.

The first campaign target is 100 stages across difficulty tiers, but the stage count does not limit the endless generator.

## UI and game states

Primary flow:

Main Menu → Hangar → Mode Select → Seed Entry (when applicable) → Loading/Generation → Run → Pause → Run or Results → Hangar.

Required screens: main menu, hangar, ship details/unlock dialog, mode selection, seed entry, settings, pause, results, records, statistics, and exit confirmation.

Every screen must be operable with mouse, keyboard, and gamepad. Focus must always be visible when navigating without a mouse. Switching input devices updates prompts without resetting the active screen or losing a selection.

## Accessibility and comfort

Include configurable sensitivity, horizontal/vertical inversion, FOV, camera distance, motion intensity, screen shake, visual effects quality, audio levels, and optional steering assistance. Reduced-motion options must disable nonessential camera bob and shake without affecting collision or timing.

## Deferred features

Online multiplayer, network leaderboards, account systems, cloud saves, monetisation, and user-generated mod tools are explicitly out of scope for the first playable release. Local seed sharing and local records are sufficient initially.

## Definition of a good run

The player can identify why a collision happened, feels the tunnel is novel rather than repeating a short pattern, believes a clean run was possible, and can retry quickly. The course surprises through geometry and combinations—not invisible hazards or arbitrary unavoidable outcomes.
