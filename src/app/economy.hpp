#pragma once

#include "app/save_profile.hpp"
#include "app/ship_catalog.hpp"

#include <cstdint>

namespace tunrun {

enum class ShipTransactionStatus {
    Purchased,
    AlreadyUnlocked,
    Equipped,
    AlreadyEquipped,
    InsufficientAetherShards,
    InsufficientSingularityCores,
    ShipLocked,
    InvalidShip
};

// Changes the in-memory profile only after validating the item and funds.
// The caller persists the candidate profile; it must roll back the in-memory
// copy if ProfileStore::save fails, so currency is never charged without a save.
[[nodiscard]] inline ShipTransactionStatus purchaseShip(
    Profile& profile, std::uint32_t shipId) noexcept {
    if (shipId >= kProfileShipCount || shipId >= kShipCatalog.size()) {
        return ShipTransactionStatus::InvalidShip;
    }
    if (profile.unlockedShips[shipId]) return ShipTransactionStatus::AlreadyUnlocked;

    const auto& item = shipDefinition(shipId);
    if (item.aetherShardCost > 0U) {
        if (profile.aetherShards < item.aetherShardCost) {
            return ShipTransactionStatus::InsufficientAetherShards;
        }
        profile.aetherShards -= item.aetherShardCost;
    } else if (item.singularityCoreCost > 0U) {
        if (profile.singularityCores < item.singularityCoreCost) {
            return ShipTransactionStatus::InsufficientSingularityCores;
        }
        profile.singularityCores -= item.singularityCoreCost;
    } else {
        return ShipTransactionStatus::InvalidShip;
    }

    profile.unlockedShips[shipId] = true;
    return ShipTransactionStatus::Purchased;
}

[[nodiscard]] inline ShipTransactionStatus equipShip(
    Profile& profile, std::uint32_t shipId) noexcept {
    if (shipId >= kProfileShipCount || shipId >= kShipCatalog.size()) {
        return ShipTransactionStatus::InvalidShip;
    }
    if (!profile.unlockedShips[shipId]) return ShipTransactionStatus::ShipLocked;
    if (profile.selectedShip == shipId) return ShipTransactionStatus::AlreadyEquipped;
    profile.selectedShip = shipId;
    return ShipTransactionStatus::Equipped;
}

} // namespace tunrun
