#pragma once

#include "app/procedural_course.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {

inline constexpr std::uint32_t kRewardGeneratorVersion = 1U;
inline constexpr double kRewardStartDistance = 20.0;
inline constexpr double kRewardSpacing = 24.0;
inline constexpr float kRewardPickupRadius = 0.82F;

enum class RewardKind : std::uint8_t {
    AetherShard,
    SingularityCore
};

struct ProceduralReward {
    std::uint32_t index = 0U;
    double distance = 0.0;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    RewardKind kind = RewardKind::AetherShard;
    std::uint32_t shardValue = 0U;
};

// Reward coordinates are derived from the same course seed but use independent
// channels, so changing pickup placement does not alter tunnel or gate layouts.
inline ProceduralReward rewardAt(std::uint64_t seed,
                                 std::uint32_t index) noexcept {
    const auto kind = index % 8U == 7U
        ? RewardKind::SingularityCore : RewardKind::AetherShard;
    const auto valueRoll = static_cast<std::uint32_t>(
        courseUnit(seed, static_cast<std::int64_t>(index), 813U) * 5.0F);
    const auto shardValue = kind == RewardKind::AetherShard
        ? 4U + (valueRoll < 5U ? valueRoll : 4U) : 0U;
    return ProceduralReward{
        index,
        kRewardStartDistance + static_cast<double>(index) * kRewardSpacing,
        courseSigned(seed, static_cast<std::int64_t>(index), 811U) * 1.45F,
        courseSigned(seed, static_cast<std::int64_t>(index), 812U) * 1.45F,
        kind,
        shardValue
    };
}

inline bool crossesRewardPlane(double previousDistance, double currentDistance,
                               const ProceduralReward& reward) noexcept {
    if (!std::isfinite(previousDistance) || !std::isfinite(currentDistance) ||
        !std::isfinite(reward.distance)) return false;
    return previousDistance <= reward.distance &&
           currentDistance >= reward.distance &&
           currentDistance > previousDistance;
}

inline bool collectsReward(float x, float y, const ProceduralReward& reward,
                           float radius = kRewardPickupRadius) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(reward.offsetX) || !std::isfinite(reward.offsetY) ||
        !std::isfinite(radius) || radius <= 0.0F) return false;
    const float dx = x - reward.offsetX;
    const float dy = y - reward.offsetY;
    return dx * dx + dy * dy <= radius * radius;
}

// Convert player coordinates from the course frame at each simulation
// endpoint into the frame used by the rendered pickup before testing contact.
// This keeps pickup physics aligned with the centerline displacement shown in
// the 3D scene on curved courses.
inline bool collectsRewardAtCourseCrossing(
    std::uint64_t seed,
    float previousX, float previousY, double previousDistance,
    float currentX, float currentY, double currentDistance,
    const ProceduralReward& reward,
    float radius = kRewardPickupRadius) noexcept {
    if (!std::isfinite(previousX) || !std::isfinite(previousY) ||
        !std::isfinite(currentX) || !std::isfinite(currentY) ||
        !std::isfinite(previousDistance) || !std::isfinite(currentDistance) ||
        !std::isfinite(reward.distance)) return false;
    if (!crossesRewardPlane(previousDistance, currentDistance, reward)) return false;

    const double travel = currentDistance - previousDistance;
    if (travel <= 1.0e-9) return false;
    const double fraction = std::clamp(
        (reward.distance - previousDistance) / travel, 0.0, 1.0);
    const auto previousSection = sampleCourse(seed, previousDistance);
    const auto currentSection = sampleCourse(seed, currentDistance);
    const auto rewardSection = sampleCourse(seed, reward.distance);

    const double worldX0 = static_cast<double>(previousX) + previousSection.centerX;
    const double worldY0 = static_cast<double>(previousY) + previousSection.centerY;
    const double worldX1 = static_cast<double>(currentX) + currentSection.centerX;
    const double worldY1 = static_cast<double>(currentY) + currentSection.centerY;
    const float rewardRelativeX = static_cast<float>(
        worldX0 + (worldX1 - worldX0) * fraction - rewardSection.centerX);
    const float rewardRelativeY = static_cast<float>(
        worldY0 + (worldY1 - worldY0) * fraction - rewardSection.centerY);
    return collectsReward(rewardRelativeX, rewardRelativeY, reward, radius);
}

// Stable canonical fingerprint for regression tests and replay compatibility.
inline std::uint64_t rewardHash(std::uint64_t seed,
                               std::uint32_t rewardCount = 256U) noexcept {
    if (rewardCount == 0U || rewardCount > 10000U) return 0U;
    std::uint64_t hash = 14695981039346656037ULL;
    const auto absorb = [&hash](std::uint64_t value) {
        for (int byte = 0; byte < 8; ++byte) {
            hash ^= value & 0xFFULL;
            hash *= 1099511628211ULL;
            value >>= 8U;
        }
    };
    absorb(kRewardGeneratorVersion);
    absorb(rewardCount);
    for (std::uint32_t i = 0U; i < rewardCount; ++i) {
        const auto reward = rewardAt(seed, i);
        absorb(reward.index);
        absorb(static_cast<std::uint64_t>(
            std::llround(reward.distance * 1000.0)));
        absorb(static_cast<std::uint64_t>(
            static_cast<std::int64_t>(std::llround(reward.offsetX * 10000.0F))));
        absorb(static_cast<std::uint64_t>(
            static_cast<std::int64_t>(std::llround(reward.offsetY * 10000.0F))));
        absorb(static_cast<std::uint64_t>(reward.kind));
        absorb(reward.shardValue);
    }
    return hash;
}

} // namespace tunrun
