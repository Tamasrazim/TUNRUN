# Procedural Rewards

Reward placement is a deterministic layer over the procedural course. It uses the active course seed while keeping its random channels separate from tunnel geometry and gate placement.

## Pickup rules

- Aether Shards appear from distance 20 and repeat every 24 distance units.
- Each Aether Shard awards 4–8 Aether Shards. The value is generated from the seed.
- Every eighth pickup is a Singularity Core. Collecting one awards one core.
- Position offsets are seeded independently for horizontal and vertical axes and stay within ±1.45 local units. The pickup is drawn relative to the same sampled course centerline used by the tunnel renderer.
- Collection is checked at the swept forward crossing plane, with the player position interpolated to that plane. A missed pickup cannot be collected later by flying backwards through it.
- The run's pickup totals are shown in the HUD and added to normal run rewards when the run ends. Total per-run rewards remain capped; currency updates use saturating arithmetic.

## Determinism and validation

`src/app/rewards.hpp` owns the reward generator, collision predicate and canonical `rewardHash`. `kRewardGeneratorVersion` identifies changes to reward rules. The automated C++ tests verify repeatability, cross-seed variation, valid reward values and bounds, forward-only plane crossing, collection radius, and rejection of non-finite positions.

This is a small, in-run reward prototype—not a claim that campaign objectives, alternate routes, timed rewards, or final progression balance are complete.
