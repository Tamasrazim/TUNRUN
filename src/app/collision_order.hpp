/*
 * What this does:
 * Keeps tunnel, gate, and moving-mine crashes ordered by the first contact in a frame.
 * This small helper is separate from drawing code so the collision rule can be tested directly.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tunrun {

enum class CollisionKind : std::uint8_t {
    Wall,
    Gate,
    GateThroat,
    Hazard
};

struct CollisionContact {
    bool collided = false;
    float fraction = 1.0F;
    float x = 0.0F;
    float y = 0.0F;
    double distance = 0.0;
    CollisionKind kind = CollisionKind::Wall;
    std::uint32_t objectIndex = 0U;
};

// Add a contact only when it occurs before the one already selected.
// Callers submit the same movement interval, so the normalized fraction is comparable.
// Exact ties deliberately keep the first candidate for deterministic crash labels.
inline void considerFirstCollision(
    CollisionContact& earliest, CollisionContact candidate) noexcept {
    if (!candidate.collided ||
        !std::isfinite(candidate.fraction) ||
        !std::isfinite(candidate.x) ||
        !std::isfinite(candidate.y) ||
        !std::isfinite(candidate.distance)) {
        return;
    }

    candidate.fraction = std::clamp(candidate.fraction, 0.0F, 1.0F);
    if (earliest.collided && candidate.fraction >= earliest.fraction) {
        return;
    }
    earliest = candidate;
}

} // namespace tunrun
