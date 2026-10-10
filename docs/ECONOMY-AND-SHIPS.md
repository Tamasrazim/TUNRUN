# Ships, Unlocks, and Economy

Status: initial balance proposal. Price and stat values are provisional until playtesting.

## Resource identity

- **Aether Shards:** common run currency.
- **Singularity Cores:** rare mastery currency.

Both are earned through play in the initial release. No real-money purchase flow is planned. The profile is authoritative for local progression, but the economy is not described as secure against save editing.

## Ship design goals

Ships must have separate visual silhouettes, cockpit anchors, engine shapes, audio profiles where feasible, and collision proxies that match their physical footprint. They are not recolours of a single mesh.

| Ship id | Name | Role | Handling trade-off | Proposed unlock |
|---|---|---|---|---|
| `driftwing` | Driftwing | Balanced starter | Predictable response; average hull | Unlocked at start |
| `wraith` | Wraith | Agile | High steering response; lower hull | 8,000 Aether Shards |
| `bulwark` | Bulwark | Heavy | Larger hull pool; slower response | 15,000 Aether Shards |
| `manta` | Manta | Stable | Smooth control; broad collision profile | 25,000 Aether Shards |
| `comet` | Comet | Racer | Strong acceleration; precision required | 38,000 Aether Shards |
| `spectre` | Spectre | Energy specialist | Boost efficiency; lower hull | 200 Singularity Cores |
| `vortex` | Vortex | Precision craft | Fast correction; advanced control | 400 Singularity Cores |
| `obsidian` | Obsidian | Prototype | Specialist hybrid; higher skill ceiling | 800 Singularity Cores + mastery objective |

The catalogue prices are now ten times the previous prototype values, as requested. Reward rates still need hands-on playtesting to confirm that unlocks remain achievable without trivialising progression. Do not implement prices as constants scattered through UI or gameplay code; use a central catalog with ids, display name, stat profile, unlock condition, and presentation metadata.

## Ship stat model

A ship profile should expose a documented range for:
- hull durability;
- lateral/vertical acceleration;
- maximum lateral/vertical speed;
- steering response and damping;
- boost duration/efficiency;
- collision radius/shape;
- visual size and cockpit/camera anchors.

Stat display should explain the practical trade-off, not show unsupported claims such as “+20% better” without an underlying parameter.

A course is validated against the selected ship's collision dimensions. If one ship cannot fit the nominal route, the validator must provide an alternate feasible passage or reject that layout for the ship/ruleset. Avoid balancing ships through invisible collision boxes that are smaller than their models without a clear design reason.

## Economy sources and sinks

Aether Shards:
- safely reachable pickups;
- optional risk/reward branches;
- distance and campaign completion rewards;
- skill/clean-run bonuses with capped values.

Singularity Cores:
- advanced campaign objectives;
- challenge milestones;
- mastery achievements;
- rare high-difficulty encounter rewards.

Sinks:
- ship unlocks;
- ship cosmetics and trails;
- optional profile/hangar customisation.

Performance upgrades, if included, require more careful balancing because they may affect course feasibility. The first playable version should prefer fixed ship trade-offs and cosmetic unlocks over a large upgrade tree.

## Rewards and exploits

- Generate rewards after the underlying geometry passes validation.
- Give each collectible a stable seed/section/entity id.
- A collectible can be claimed only once per run.
- Results and checkpoint payouts are idempotent.
- Retrying a seed should not duplicate campaign completion rewards unintentionally; repeatable and first-time rewards must be explicit.
- Daily challenge rules define whether retries grant currency; no daily system may depend on a local clock alone for a competitive online claim.
- Every wallet transaction must follow [Save Schema](./SAVE-SCHEMA.md).

## Hangar behavior

The hangar allows inspection, preview rotation, stat comparison, unlock requirement inspection, cosmetic preview, purchase confirmation, and equip. Purchasing and equipping are separate actions. If the player cannot afford a ship, show the missing amount without changing their balance.

After a successful unlock, persist the unlock and wallet deduction before showing the durable success state. If persistence fails, communicate the failure and do not claim the unlock has been saved.
