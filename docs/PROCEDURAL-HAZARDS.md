# Procedural Moving Hazards

TUNRUN now generates a deterministic sequence of moving mine hazards alongside the tunnel and aperture gates.

## Runtime rules

- The first mine appears around distance 88–98. Following mines are spaced 84 distance units apart.
- Each mine has a seed-derived baseline, horizontal and vertical amplitudes, speed, phase, collision radius, and one of four independent motion profiles: lateral sweep, vertical sweep, elliptic orbit, or figure-eight.
- Motion is a pure function of the generated parameters and per-run elapsed clock. The new motion-family choice uses its own deterministic seed channel, separate from the cosmetic shell family; identical seeds and times reproduce identical centers.
- The visible shell may be Orbital, Prism, Rotor, or Cross, while its trajectory type is selected independently. The HUD names both layers so the movement pattern is readable before arrival.
- All four trajectory profiles stay inside the same declared movement envelope and retain a shared spherical collision proxy. Their movement patterns differ; this version does not claim different collision shapes.
- Collision uses a frame-to-frame swept 3D closest-approach check in relative craft/mine coordinates. Both endpoints are transformed from the player's current cross-section into the mine's course-relative frame, and mine motion is interpolated between frame endpoint positions. Collision is evaluated even on rendered frames that do not advance fixed-step forward distance.
- A collision records the normal run outcome, pays the same capped rewards, and identifies the hazard impact on the crash screen. Retrying the same seed resets the run clock so the hazard route repeats.

## Determinism and validation

`src/app/hazards.hpp` owns generator version 3, hazard sampling, faster deterministic oscillation (frequency 0.95–1.80), contact testing, a canonical hazard hash, and a bounded parameter/reaction-separation validator. The unit tests cover stable generation, stable motion for identical times, cross-seed hash differences, contact and near-miss cases, forward-only crossing, motion bounds, and separation from gate reaction windows.

The validator screens generated parameters, motion-family values, and hazard-to-gate spacing; the unit suite runs a 24-seed regression batch plus all-four-family coverage, motion-bound checks, swept near-edge, near-miss, stationary-tick, curved-centerline and invalid-input cases. Gate and mine collision use the sampled course centerline to align physics with rendered obstacle positions. Swept mine collision interpolates lateral motion linearly between frame endpoints, so it remains an approximation rather than a formal continuous collision proof for arbitrary frame durations. It also does not prove that every mine can be avoided by every ship. Visual warning timing and actual feel still require hardware playtesting.
