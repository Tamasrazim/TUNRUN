# Procedural Generation Specification

## Implemented generator baseline

Generator v1 uses a stateless 64-bit hash stream keyed by the run seed, control-node index, and parameter channel. Control nodes are spaced 18 normalized course units apart. Catmull-Rom interpolation produces bounded center offsets, tunnel radii, and cross-section twist without mutable global RNG state. The same analytic sampler feeds both the wireframe renderer and the tunnel-wall collision check. A canonical quantised parameter hash and bounded-range validator are available in code and surfaced in the Seed Lab.

Obstacle generator v8 expands deterministic aperture gates into four gameplay families and four actual throat waveform families: Single S, Double S, Helical Weave, and Split Wave. Each gate retains an 18-unit-per-side tapered throat and a 9-unit narrow core either side of the gate plane. Independently seeded lateral/vertical amplitudes remain bounded at 1.58–1.90 and 0.20–0.35 units. Split Wave uses a single/double-harmonic lateral blend and a single/triple-harmonic vertical blend so it retains the minimum lateral deflection needed through the narrow core. The four waveform families redistribute those amplitudes across one, two, or blended sine harmonics. Each wave evaluates to zero at both sleeve ends and at the gate plane, and convex wave blends preserve the unit envelope. A smooth easing envelope affects only the outer quarter of each sleeve, keeping the core shape intact while making the bend's first derivative zero where it rejoins the main tunnel. Thus passage topology varies without widening the prior maximum bend envelope. Rendering, swept collision, route guidance, and camera clearance all consume the same sampled centerline; family and amplitudes are included in the obstacle hash. Route guidance estimates lateral/vertical centreline velocity from the shared sampler and feeds it into the controller so its damping follows the moving throat rather than opposing it. Their aperture sizes and offset ranges are type-specific, and type selection uses its own seed channel. The renderer gives each gate family a distinct visual cue; the in-run HUD reports the gameplay gate type, support silhouette, and selected actual throat waveform. The validator checks each family's radius envelope, finite parameters, bounds, opening clearance, and minimum reaction spacing. A pairwise reachability screen checks adjacent gate centers against each ship's speed and acceleration profile at maximum forward (boost) speed, counting both openings as endpoint tolerance. A second, fixed-step route probe now propagates actual lateral position and velocity across consecutive gates using the gameplay `updateFlight()` physics, the ship's boost/handling profile, a bounded feedback pilot, conservative minimum-tunnel clearance, and the same interpolated gate-plane crossing test used by the run loop. It reports the first failed gate, minimum aperture clearance, maximum lateral offset, and simulation step count. This is a repeatable feasible-trajectory witness for a specific controller, not an exhaustive reachable-state graph or a proof that every possible trajectory is recoverable. An independent obstacle hash includes the obstacle generator version and quantised gate payload, so obstacle-rule changes do not reuse the tunnel's geometry hash. Seed Lab reports obstacle counts and both the pairwise envelope and state-propagated route result.

The generator's deeper narrow core and more pronounced S-bend are shared by the visible throat, collision checks, route validation, and camera occlusion. They reduce the view through narrow apertures rather than relying on a decorative fog panel. This remains an early world-generation baseline, not the complete generator. These are four parameterised gate families, not a claim of broad structural or moving-hazard variety. The renderer now derives a local orthonormal right/up/tangent frame from the seeded centreline and cross-section twist. Tunnel rings, gates, pickups, mine details, the third-person ship and both camera positions use this frame so bends and twists orient the visible world rather than merely shifting ring centres. The tangent-aligned tunnel frame remains a rendering-space basis, while player flight offsets continue to use the documented course-relative model. Gate-throat cross-sections are defined in that same local model and are shared across rendering, swept collision, route guidance, and camera clearance. Distance fog lowers the contrast of far tunnel surfaces and objects without opening holes in the continuous wall. Compositional obstacle grammars beyond the current gates, richer multi-part moving-hazard choreography, reward-aware alternate routes, asynchronous streaming and a full reachable-state graph validator remain outstanding. Mine motion now has four deterministic trajectory profiles; this is trajectory variation, not a claim of complex multi-obstacle timelines. The fixed-step route probe checks one concrete trajectory and is not exhaustive.

## Objective

Generate continuously new tunnel geometry and obstacle arrangements from a run seed. This must be a genuine procedural system—not a sequence of finished obstacle prefabs selected from a small list.

The system has three separate responsibilities:

1. **Generation:** construct possible geometry and behavior.
2. **Validation:** reject or repair content that violates physical/playability limits.
3. **Streaming:** prepare accepted content before the player reaches it.

## Reproducibility contract

Each run has:
- a 64-bit root seed;
- a generator version;
- a ruleset/difficulty configuration;
- a ship physics profile identifier.

Text seed entry normalizes ASCII case and repeated whitespace before hashing into a root seed; a `0x`-prefixed or 16-character hexadecimal seed is parsed literally. The chosen root seed and run serial are persisted so the selected course identity can be reconstructed after restart. Course identity includes the root seed, generator version, ruleset, and relevant generation settings. The same identity must recreate the same canonical section parameters and obstacle course.

Use deterministic pseudorandom streams derived independently from (root seed, generator version, section index, subsystem id). Separate streams are required for tunnel geometry, structural topology, obstacle motion, resources, and cosmetic variation. The renderer uses its own tunnel-visual channel to choose five repeating wall-architecture motifs in 24-unit zones; changing a decorative style must not rearrange obstacle placement or alter collision geometry. Adding a visual particle must not rearrange obstacle placement.

Quantise the generated canonical parameters before mesh construction. The promise is reproducibility of course data and gameplay placements; tiny renderer differences across drivers do not invalidate that promise.

## Tunnel geometry model

Represent the tunnel as a 3D centerline parameterised by distance. Construct a stable local frame along it so cross-sections can twist without sudden frame flips.

The centerline may combine bounded low-frequency coherent noise with generated spline/control parameters. Noise is for smooth variation, not unrestricted high-frequency randomness. Apply explicit limits for:
- maximum lateral displacement;
- curvature and curvature change;
- vertical slope;
- twist rate;
- minimum and maximum tunnel radius;
- radius change per unit distance;
- local cross-section slope and surface distortion.

Cross-sections may be circular, elliptical, asymmetric, polygonal, or perturbed by bounded angular harmonics. Parameter ranges must always preserve a minimum traversable opening and collision surface continuity.

Generation must support compound and changing geometry: a curved section can twist, taper, flare, or morph into a differently shaped entrance. The transition must preserve position and tangent continuity and avoid cracks or sudden collision jumps.

## Entrances and transitions

Entrances are generated as geometry, not picked as a complete prebuilt picture. The generator varies aperture profile, rim thickness, number/shape of structural ribs, asymmetry, offset, depth, local rotation, and the way the entrance blends into the previous and next tunnel surfaces.

A generated entrance may lead into a different cross-section or tunnel orientation. The transition must be continuous enough for the craft's speed and movement limits. The generator may create unusual entry silhouettes, but must not place decorative geometry inside the required flight clearance unless it is intentionally a validated hazard.

## Procedural structures and obstacles

Use reusable low-level geometry operations—extrusion, ring segments, panels, struts, swept beams, bounded deformation, fractured pieces, and generated surface fields—to build higher-level structures from new parameter combinations.

The main unit of variety is not a model id; it is the generated structure description. A structure description contains topology, dimensions, local transform, material parameters, motion model, collision representation, and warning cues.

Examples of parameter dimensions include:
- number and thickness of structural elements;
- opening count, position, angle, and aspect ratio;
- asymmetry and bounded deformation;
- independent rotation axes and phase offsets;
- translation, extension, retraction, oscillation, or sequential movement;
- static/dynamic component mix;
- placement in the local tunnel frame;
- material pattern and lighting, kept separate from gameplay geometry.

These dimensions can combine freely within constraints. No fixed repeating sequence such as “curve, ring, crusher, helix” should drive an ordinary run. Families may exist as generation grammars to control structural coherence, but the actual geometry and combinations must be parameterised and numerous.

## Noise usage

Use coherent seeded noise for geometry and surface variation where continuous spatial correlation makes sense. Use seeded random draws for discrete topology and choices. Use explicit periodic functions or deterministic motion parameters for moving obstacles.

Do not use raw per-vertex random noise to generate primary tunnel surfaces: that tends to create jittery, jagged, unfair geometry. Bounded high-frequency detail may decorate surfaces but must not alter the collision corridor significantly.

## Procedural resources

Aether Shards and rare Singularity Core placements are generated after geometry and obstacle validation. Resource placement should reward controlled optional risk while leaving the baseline route viable. Rare rewards may be placed in difficult but reachable pockets; never use reward placement as proof that a route is safe.

Rewards must be tied to stable section indices and deterministic streams so the course can be reproduced.

## Playability validator

The validator should operate on the actual generated collision representation and player movement limits, not just visual meshes.

For every section:
1. Confirm mesh/collision continuity with adjacent sections.
2. Build a conservative collision/clearance representation.
3. Evaluate a look-ahead reachable-state graph over tunnel distance, lateral/vertical position, and bounded steering velocity.
4. Include dynamic obstacle positions at their relevant times.
5. Reject if no viable trajectory remains or if warning distance is below the allowed reaction-time envelope.
6. Verify ship-specific clearance for all supported ship profiles.
7. Ensure resources do not occlude the required route or create unsafe collision ambiguity.
8. Record the seed, section index, failing invariant, and generation parameters for debugging.

The validator must not force every section into an obvious common pattern. It enforces feasibility and readability, not a hand-authored route sequence.

If a candidate fails, regenerate the relevant subsystem using a deterministic retry stream and bounded retry count. If it continues to fail, fall back to a conservative procedural construction rule for that section and record the fallback. This is a safety net, not the normal content path.

## Difficulty without obvious scripting

Difficulty should emerge from bounded parameters such as:
- width and clearance relative to the selected ship;
- curvature and twist rate;
- number and timing relationships of moving components;
- depth/visibility and warning distance;
- number of viable paths and the precision required;
- interaction of independent geometry and motion systems.

Difficulty tiers change parameter envelopes and validator thresholds, not a fixed obstacle schedule. Hard sections may be uncommon or require skill, but must remain possible and offer readable warning.

Do not increase every difficulty dimension at once. Track each dimension so balancing can identify why a section was difficult.

## Streaming and performance

Generate several seconds of course ahead of the player, with the exact look-ahead distance driven by maximum speed and worst-case section complexity. Keep a safety buffer of already-validated sections.

CPU generation and validation may run on worker threads using immutable input parameters. GPU mesh creation/destruction must be marshalled safely to the rendering thread. No generation job may mutate live collision data midway through a frame.

When frame time spikes, increase pre-generation budget or simplify non-gameplay detail; never skip obstacle collision or spawn an unvalidated section just to keep moving.

Generated sections behind the player can be unloaded once they are outside the rollback/ghost-replay retention window. Keep compact canonical seed/parameter records for deterministic replay.

## Required test corpus

- Known fixed seeds for unit/regression tests.
- Large random seed batches across all difficulty tiers and ship profiles.
- Boundary-value tests for smallest openings, maximum curvature, extreme twist, longest moving hazards, and section seams.
- Dynamic collision tests with different obstacle phases.
- Cross-run checks that the same seed produces the same canonical course hash.
- Performance tests at the highest speed and densest allowed geometry.
- Failure reports that reproduce by seed, generator version, section index, and ship id.


## Gate support silhouettes (visual generator v1)

Gate gameplay generation remains separate from its decorative construction. A second seeded channel (`191`) assigns every gate one of five support designs: **Radial Cage**, **Segmented Crown**, **Chevron Brace**, **Twin Rails**, or **Split Clamps**. Each style uses the existing gate aperture as its reference and draws lines only on the surrounding annulus; the minimum visual inset is 0.18 world units beyond the aperture edge.

These styles do not modify `GateKind`, aperture radius, offsets, spacing, throat geometry, scoring, reachability checks, collision tests, or the obstacle hash. Replaying the same seed and gate index yields the same silhouette. The in-run gate cue reports both the gameplay gate kind and the visual structure. Unit tests cover determinism, name/index validity and family coverage over 512 gates; they do not replace a manual in-game visual review.


## Moving-mine proximity warnings

The warning profile is derived from the same distance and elapsed run clock as the rendered mine. Its first segmented ring becomes visible inside 54 course units; at 22 units it switches to the urgent color/state and adds a second ring. Pulse frequency increases from 1.3 Hz to 4.3 Hz with approach distance. The warning ring is centered on the same sampled tunnel frame and animated mine center as the visible model, so it tracks a mine through a curved tunnel rather than using a straight-world projection.

The warning profile is rendering/HUD feedback only: it does not change the hazard seed channels, trajectory, radius, collision tests, or hazard hash. Unit tests cover activation thresholds, rear grace, invalid numeric inputs, repeatability, and bounded pulse/ring values.


## Expanded mine shell library (visual generator v1)

The deterministic mine shell channel now selects six distinct procedural meshes: Orbital, Prism, Rotor, Cross, Halo Array, and Shard Cluster. Halo Array draws three intersecting depth-aware hoops around a small core; Shard Cluster builds six triangular crystal fins around a compact hub. They have separate palettes and geometry rather than relying only on recoloring the original four shells. All mesh selection remains in visual channel `910`; hazard movement remains on channel `911`, and physics parameters remain in their existing channels. This is a visual-only extension: hazard generator version 3, hazardHash, movement envelopes, and the shared spherical collision proxy are unchanged.

Coverage tests check all six shell families over 512 seeded hazard indices, verify that names resolve, and explicitly reject invalid family enum values. They are generator-level tests; a manual in-game readability and performance review remains necessary.


The Seed Lab and automated tests cover the four throat waveform families over 512 seeded gate indices. This verifies deterministic selection, waveform bounds, gate-plane alignment, and that the section sampler uses the same profile returned to downstream systems; a manual flight run remains necessary to judge the four shapes at speed.
