# Procedural Moving Hazards

TUNRUN now generates a deterministic sequence of moving mine hazards alongside the tunnel and aperture gates.

## Runtime rules

- The first mine appears around distance 88–98. Following mines are spaced 84 distance units apart.
- Each mine has a seed-derived baseline, horizontal and vertical oscillation amplitudes, speed, phase, and collision radius.
- Motion is a pure function of the per-run elapsed clock and the generated parameters. Given the same seed and elapsed time, it returns the same center.
- A small red wireframe sphere and crosshair lines identify each mine in the tunnel. The HUD reports the distance to the next hazard.
- On a forward crossing of the hazard plane, the craft position and run clock are interpolated between simulation frames before collision is evaluated.
- A collision records the normal run outcome, pays the same capped rewards, and identifies the hazard impact on the crash screen. Retrying the same seed resets the run clock so the hazard route repeats.

## Determinism and validation

`src/app/hazards.hpp` owns generator version 1, hazard sampling, motion evaluation, contact testing, a canonical hazard hash, and a bounded parameter/reaction-separation validator. The unit tests cover stable generation, stable motion for identical times, cross-seed hash differences, contact and near-miss cases, forward-only crossing, motion bounds, and separation from gate reaction windows.

The validator only screens generated parameters and hazard-to-gate spacing. It does not prove that every moving mine can be avoided by every ship, and the current mine is a plane-crossing prototype rather than a full continuous 3D swept-sphere solver. Visual warning timing and actual feel still require hardware playtesting.
