#pragma once

#include <array>
#include <cstdint>

namespace tunrun {

struct ShipDefinition {
    const char* name;
    std::uint64_t aetherShardCost;
    std::uint64_t singularityCoreCost;
    float speedMultiplier;
    float accelerationMultiplier;
    float boostDrainMultiplier;
    float energyRegenerationMultiplier;
};

// One catalogue drives both hangar prices and actual flight handling.
// Unlocks are local progression, not an online/anti-cheat boundary.
inline constexpr std::array<ShipDefinition, 8> kShipCatalog{{
    {"DRIFTWING", 0U,    0U, 1.00F, 1.05F, 1.00F, 1.00F},
    {"WRAITH",    800U,  0U, 1.12F, 1.05F, 1.15F, 0.90F},
    {"BULWARK",   1500U, 0U, 0.84F, 0.95F, 0.75F, 1.25F},
    {"MANTA",     2500U, 0U, 0.96F, 1.28F, 1.00F, 0.95F},
    {"COMET",     3800U, 0U, 1.25F, 0.90F, 1.35F, 0.75F},
    {"SPECTRE",   0U,    20U, 1.04F, 1.08F, 0.80F, 1.10F},
    {"VORTEX",    0U,    40U, 1.12F, 1.20F, 1.05F, 0.90F},
    {"OBSIDIAN",  0U,    80U, 0.94F, 0.98F, 0.65F, 1.35F}
}};

inline constexpr std::uint32_t kStarterShipId = 0U;

[[nodiscard]] inline constexpr const ShipDefinition& shipDefinition(
    std::uint32_t id) noexcept {
    return kShipCatalog[id < kShipCatalog.size() ? id : kStarterShipId];
}

} // namespace tunrun
