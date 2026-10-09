# Procedural Moving Hazards

TUNRUN now generates a deterministic sequence of moving mine hazards alongside the tunnel and aperture gates.

## Runtime rules

- The first mine appears around distance 88–98. Following mines are spaced 84 distance units apart.
- Each mine has a seed-derived baseline, horizontal and vertical oscillation amplitudes, speed, phase, and collision radius.
- Motion is a pure function of the per-run elapsed clock and the generated parameters. Given the same seed and elapsed time, it returns the same center.
- A small red wireframe sphere and crosshair lines identify each mine in the tunnel. The HUD reports the distance to the next hazard.
- Collision uses a frame-to-frame swept 3D closest-approach check in relative craft/mine coordinates. Both endpoints are transformed from the player's current cross-section into the mine's course-relative frame, and mine motion is interpolated between frame endpoint positions. Collision is evaluated even on rendered frames that do not advance fixed-step forward distance.
- A collision records the normal run outcome, pays the same capped rewards, and identifies the hazard impact on the crash screen. Retrying the same seed resets the run clock so the hazard route repeats.

## Determinism and validation

`src/app/hazards.hpp` owns generator version 2, hazard sampling, faster deterministic oscillation (frequency 0.95–1.80), contact testing, a canonical hazard hash, and a bounded parameter/reaction-separation validator. The unit tests cover stable generation, stable motion for identical times, cross-seed hash differences, contact and near-miss cases, forward-only crossing, motion bounds, and separation from gate reaction windows.

The validator screens generated parameters and hazard-to-gate spacing; the unit suite runs a 24-seed regression batch plus swept near-edge, near-miss, stationary-tick, curved-centerline and invalid-input cases. Gate and mine collision use the sampled course centerline to align physics with rendered obstacle positions. Swept mine collision interpolates lateral motion linearly between frame endpoints, so it remains an approximation rather than a formal continuous collision proof for arbitrary frame durations. It also does not prove that every mine can be avoided by every ship. Visual warning timing and actual feel still require hardware playtesting.
