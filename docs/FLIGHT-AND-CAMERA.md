# Flight, Physics, and Camera Specification

Status: **prototype in progress**. Flight now advances along a seeded analytic course using a fixed 120 Hz lateral/vertical controller. Boost energy, precision steering, forward distance, wall collision, and a retry path are implemented. Tuning, camera-frame upgrades, and broader acceptance testing remain.

## Flight model

Forward progress is automatic by default. The craft moves along the generated course direction while the player controls its offset and orientation relative to a stable local tunnel frame. It should feel like piloting a spacecraft, not selecting one of three lanes.

Separate the authoritative craft simulation from camera transforms. The local tunnel frame supplies forward, right, and up axes; these axes must be continuous through curves and twists. Never derive authoritative movement from the rendered camera.

Simulation should use a fixed timestep or controlled accumulator. Render frames interpolate visual transforms; frame rate must not change movement, boost drain, obstacle phase, or collision outcome.

### Motion components

- Forward speed, target speed, acceleration, braking/precision mode, boost, and dash.
- Dash is an edge-triggered 0.24-second forward burst (19 units/s, or 24 while also boosting), costs 28 energy, and has a 1.20-second cooldown. It cannot activate while precision mode is held and does not repeat while the key/button remains down. A key used to confirm a menu action is treated as already held when the run starts, preventing an accidental dash on entry.
- Horizontal and vertical steering with bounded rate and acceleration.
- Angular response and banking based on steering and tunnel frame rotation.
- Hull integrity and collision response.
- Craft-specific handling parameters and collision dimensions.
- Optional assists: steering stabilisation, reduced camera motion, and forgiving input response.

Flight parameters must be data-driven by a ship profile. Do not hard-code ship identity into physics branches.

The current prototype has a 120 Hz fixed-step lateral/vertical simulation. W accelerates above cruise, S brakes toward a slow forward crawl, and A/B provide lateral steering (D remains a right-steer alias). Braking suppresses boost, takes priority over precision speed, and blocks new dash activations; an already-triggered dash completes its committed burst. Mouse look is independent of ship yaw/pitch. The TPP camera base stays behind the course tangent instead of swinging sideways behind the ship's nose. Dash timing, energy cost, and cooldown advance only on fixed simulation steps, so render rate does not change the burst duration. Filled tunnel panels, centerline bends, wall checks, gate contacts, moving mines, and pickups use the same seeded course sampler. The renderer creates orthonormal tangent-aligned right/up frames with generated twist for tunnel rings, gates, pickups, mine details, TPP ship geometry, and both camera positions. Ring frames are cached once per visible cross-section. The gate throat is gameplay geometry rather than a decorative sleeve: its narrow core stays constricted for longer before smoothly flaring into the main tunnel. One shared sampled centre/radius feeds rendering, swept collision, route-graph steering/collision, and camera placement. The throat's deterministic shallow S-bend is used by gameplay and presentation together. The FPP origin and TPP chase origin clamp to the throat when occupying it, and a sampled camera-ray limit shortens look rays that cross either a curved outer wall or the smaller throat sleeve; tests cover rays that remain inside the outer tunnel but would pass through solid throat geometry. The exact gate plane continues to use interpolated crossing collision. Opaque tunnel panels, gate details, mines, pickups, and wormhole filaments fade toward the near-black scene background sooner, reducing how far into an opening the player can inspect. Mine silhouettes use four seed-selected cosmetic families; those presentation variants do not alter mine motion or collision. A critically damped chase spring and hands-on hardware camera acceptance remain future work.

## Initial prototype tuning targets

These are starting points for measurement, not final balance:

| Parameter | Prototype target |
|---|---|
| Simulation step | 1/120 s where the frame budget permits; deterministic fixed-step accumulator |
| Cruise speed | 1 tunnel-distance unit per simulation second (normalised); tune after the first playable course |
| Boost speed | 1.35–1.6× cruise, balanced against visibility and warning distance |
| Steering response | Smooth acceleration with capped lateral/vertical velocity; no instantaneous teleporting |
| Camera blend | 0.25–0.45 s between FPP and TPP |
| Input focus loss | Pause immediately and clear held inputs |
| Collision grace | No hidden invulnerability from camera switching; any collision grace must be explicitly designed and tested |

Use normalised simulation units initially. Define a single world-scale conversion for rendering/audio rather than scattering arbitrary scales through gameplay code.

## Control interpretation

The input service emits normalised intent:
- SteerX and SteerY in [-1, 1].
- Boost and Dash as edge/held actions as appropriate.
- Brake/Precision as a held action.
- CameraSwitch and Pause as edge-triggered actions.
- UI navigation as directional/confirm/back actions.

Keyboard is digital input with a consistent ramp/response curve. Gamepad sticks use configurable dead-zone and response curves. Mouse look uses relative delta scaled by saved sensitivity and changes the camera direction only; it must not steer the spacecraft or depend on repeated pointer warping. W accelerates, S brakes, A steers left and B steers right (D is also accepted as a right-steer alias). With no speed key held the craft returns to its established cruise speed.

## Collision model

Visual geometry and gameplay collision must share a generated source description. Use conservative collision volumes for procedural shapes. The craft collision shape must be smaller than its full decorative silhouette only if that rule is visible and consistent; document the chosen collision proxy in each ship profile.

Collision should resolve against tunnel boundaries and generated obstacles. A camera crossing a wall is not necessarily a gameplay collision, but camera clipping in TPP must be corrected separately.

On a collision:
1. Resolve position/velocity without tunnelling through thin structures.
2. Apply hull damage and audiovisual feedback.
3. Record obstacle/section identity for debugging.
4. Prevent multiple damage events from one overlap unless the obstacle intentionally applies continuous damage.
5. If hull reaches zero, stop the run and enter Results through an explicit state transition.

## FPP camera

- Attach the viewpoint to a defined cockpit/pilot anchor.
- Clamp the actual camera origin to the camera's sampled cross-section; a valid ship position in the current section does not guarantee that a camera placed in a narrower rear section will fit.
- Follow the craft's orientation in the local tunnel frame.
- Keep a minimum look-ahead view; use warning shapes/lighting to make hazards readable.
- Avoid extreme camera roll, zoom, or shake by default.
- Permit FOV and comfort settings inside tested limits.
- Ship geometry must not obstruct the view.

## TPP camera

- Chase the same craft transform with a spring-damped or critically damped follow model.
- Look ahead toward the upcoming flight path, not only directly at the craft.
- Use camera obstruction tests against tunnel geometry and smoothly shorten/offset the camera when needed.
- Clamp the camera to safe tunnel/camera volumes near tight curves.
- Never modify craft physics to compensate for camera movement.

## Switching cameras

Camera switch is presentation-only:
- Do not reset seed, section index, position, velocity, obstacle phase, score, or collision state.
- Smoothly blend position/orientation/FOV.
- If TPP cannot find a clear view, temporarily use a safer offset without changing player control.
- Both modes must preserve equivalent steering intent and must not give different collision sizes.

## Boost and dash

Boost consumes an energy resource and increases forward speed temporarily. Energy dash is a short controlled burst/ability with a defined cooldown or energy cost. Their final relationship must be decided in the first playable prototype.

The generator's warning-time validator must use maximum achievable boost speed. Boost must not allow a player to bypass every obstacle or invalidate seed fairness.

## Flight acceptance criteria

- Same input sequence and simulation configuration produce the same craft trajectory within the deterministic test environment.
- 30, 60, 120, and uncapped render rates do not materially change gameplay outcomes.
- Switching FPP/TPP changes only the camera.
- Mouse look rotates the camera without changing craft trajectory or being dragged by ship heading; W accelerates, S brakes even while boost/precision is held (an active dash completes), and released speed control returns toward cruise.
- The first-person camera is clamped inside its own tunnel cross-section, not merely the ship's current section; the third-person camera remains radially within the sampled rear section and follows the course direction through ship rotations.
- The finite-depth throat center/radius must match across rendering, swept live collision, route guidance, and FPP/TPP camera clearance. Passing through its center plane does not excuse intersecting the tapered sleeve before or after that plane.
- Tunnel surfaces and objects remain opaque as appropriate but fade toward the near-black background with distance, reducing sharp visibility through narrow apertures.
- Focus loss and pause clear inputs, release capture, and freeze hazard time.
- No craft can exploit camera switching to cross obstacles.
- Collision cases include walls, thin barriers, corners, moving obstacles, and high-speed contact.


## Dash feedback

During the 0.24-second dash, short cyan streaks animate along the tunnel perimeter. The HUD switches to `DASH ACTIVE`, then shows cooldown or the minimum energy needed before another dash. These are visual cues only; flight speed and dash timing remain controlled by the fixed-step physics.
