# Art, Audio, and Asset Direction

## Visual identity

The reference is the original Velocity Run tunnel-runner SVG: forward perspective, expanding spatial frames, fast-moving geometry, and a small spacecraft silhouette. TUNRUN expands that composition into a fully navigable 3D environment.

Core palette and readability:
- graphite / near-black environment;
- silver and icy-white geometry;
- restrained luminous edge cues;
- strong silhouette contrast between hazards, tunnel boundaries, rewards, bots, and the player;
- minimal HUD that does not obscure the center of the view.

Critical hazards must remain readable against both bright and dark backgrounds. Do not depend on hue alone to distinguish states. Use shape, motion, edge contrast, and optional labels/indicators.

## Generated geometry and identity

Procedural variation should feel intentional. Combine a small set of low-level geometry operations to produce a broad space of structures with consistent visual language. Avoid making all generation look like primitive cubes arranged randomly.

Each generated gameplay object separates:
1. canonical gameplay parameters;
2. visible mesh/material parameters;
3. collision representation;
4. motion description;
5. warning and readability cues.

Material randomness and particle effects must not affect gameplay determinism.

## Player ships

The native prototype now renders eight procedural third-person wireframe silhouettes and matching 2D hangar previews directly through raylib, one for each catalogue entry: DRIFTWING, WRAITH, BULWARK, MANTA, COMET, SPECTRE, VORTEX, and OBSIDIAN. These are readable development silhouettes, not final production meshes or materials.

Ships require clear silhouettes at typical gameplay distances. Each needs:
- main hull;
- wings/fins or other identifying structure;
- engine/exhaust representation;
- FPP cockpit/pilot anchor;
- TPP chase-camera anchor;
- collision proxy and dimensions;
- hangar preview presentation;
- optional damage/boost states.

Cosmetic changes may alter paint, trim, exhaust trail, and small decoration, but must not silently change collision or handling.

## Lighting and motion effects

Use lighting to reinforce tunnel depth and speed, not to hide geometry. Bloom, motion streaks, fog, distortion, and screen shake must have quality/comfort controls. Reduced-motion mode disables nonessential camera shake, aggressive zoom pulses, and UI motion.

## Audio design

Planned audio categories:
- propulsion / engine loop;
- boost and dash;
- tunnel ambience that responds to speed and geometry;
- obstacle warning cues;
- collision and hull damage;
- pickup and currency reward;
- menu and hangar UI;
- bot movement/event cues;
- results and progression feedback.

Audio should be generated or licensed for distribution. Do not ship copyrighted music or effects without rights. Mix for headphones and speakers; critical warnings should be distinguishable without being painfully loud. Provide master, music, engine/ambience, effects, and UI volume controls where applicable.

## Asset pipeline

- Keep source models and textures in a documented source format.
- Export to runtime formats through reproducible steps.
- Record licenses/attribution for third-party assets.
- Validate referenced assets exist and paths are relative to the packaged game.
- Keep runtime assets separate from development-only source files and tools.
- Do not embed external web requests or remote dependencies required for gameplay.

## Performance fallback

Quality settings should reduce nonessential detail, particles, fog, and expensive screen effects before weakening tunnel collision or skipping obstacle simulation. High-contrast hazard edges and core route readability must remain available at every quality preset.
