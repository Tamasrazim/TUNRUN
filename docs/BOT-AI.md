# Bot and Ghost AI Specification

## Rule

Bots must navigate the same generated space, collision representation, and hazard timing rules as the player. They are not allowed to teleport, ignore collision, move through walls, or receive secret access to safe routes unavailable to the player.

## Entity categories

### Rival pilots
Compete for time, position, or score. They choose routes, steer in 3D, manage speed, and can make errors appropriate to their difficulty.

### Environmental drones
Populate the environment as patrols, escorts, observers, or moving entities that cross the flight path. Their motion must remain predictable enough to read at high speed.

### Pursuit/event entities
Used in specific gameplay events. Pursuit uses legal movement and a bounded catch-up model; it cannot teleport into the player's collision volume.

### Ghosts
Replay a recorded run as a non-physical visual competitor. Ghosts do not block routes or collide with the player. Recordings include generator version, physics version, ship profile, seed identity, and sampled trajectory data.

## AI navigation pipeline

1. Read the validated course data in a configurable look-ahead window.
2. Construct candidate flight corridors from tunnel clearance and obstacle state.
3. Predict moving geometry across a time horizon.
4. Score corridors for safety, travel time, reward opportunity, and difficulty preference.
5. Choose a target corridor/trajectory.
6. Produce legal steering, boost, and precision-mode actions.
7. Monitor tracking error and replan when the environment changes.
8. Resolve collisions through the same physics rules used by the player.

Use hierarchical planning: a coarse route target over the next few sections, plus local avoidance for nearby obstacles. A bot should not need to solve the entire endless course at once.

## Difficulty profiles

| Profile | Navigation behavior |
|---|---|
| Rookie | Short prediction horizon, cautious routes, slower recovery from errors |
| Racer | Balanced risk and route selection; competes consistently |
| Ace | Longer prediction, fast corrections, good resource use |
| Daredevil | Prefers optional risky paths when they remain physically feasible |
| Adaptive | Adjusts aggression within a bounded envelope based on recent performance |

Difficulty must change prediction horizon, reaction delay, correction quality, route risk preference, and consistency. It must not change collision rules or grant extra clearance.

## AI geometry and ship profiles

AI uses the actual collision profile and movement limits of its ship. If a rival's craft is too wide for a generated route, it must choose another route or collide naturally. Course validation for bots may use a conservative bot profile, but cannot promise that every bot will succeed on every course.

Bot ship models and human ship models can share geometry assets and flight-profile data when appropriate.

## Procedural spawning

Spawning uses its own deterministic random stream and stable section/entity ids. Cosmetic animation or particle effects cannot change bot spawn positions. Bot spawns must satisfy separation checks so they do not appear inside the player, walls, or an obstacle.

Population is budgeted by performance tier and difficulty. If the frame budget is threatened, reduce distant non-interactive entities before reducing collision accuracy or skipping simulation state.

## Bot interactions

Initial release:
- No online multiplayer.
- No player-vs-player network authority.
- Local AI pilots can race, share a route, or appear in events.
- Bots may compete for pickup opportunities but cannot delete a resource before it is physically reached.
- Bot outcomes should be reproducible under a matching generator/physics version when AI determinism is enabled.

## Ghost recording format

A compact recording stores metadata plus periodic samples of craft transform, orientation, speed, and selected visible events. Interpolate samples for rendering; do not run a ghost as authoritative physics. Reject or clearly label a ghost recorded with an incompatible course or physics version.

## AI acceptance criteria

- Bot transforms stay within legal movement limits.
- Replaying the same seed and deterministic AI configuration reproduces canonical decisions or trajectory samples within documented tolerances.
- Bots avoid generated obstacles without modifying collision rules.
- Spawns satisfy minimum clearance from player and geometry.
- Difficulty profiles are measurably distinct in prediction/risk/consistency.
- Ghosts do not collide with or block the player.
