#pragma once

#include "app/procedural_course.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tunrun {

inline constexpr std::uint32_t kScoringRulesVersion = 1U;
inline constexpr std::uint64_t kScoreCap = 9000000000000000ULL;

struct RunScore {
    std::uint64_t total = 0U;
    std::uint64_t gatesPassed = 0U;
    std::uint64_t cleanPasses = 0U;
    std::uint64_t combo = 0U;
    std::uint64_t bestCombo = 0U;
    std::uint64_t lastAward = 0U;
};

struct GateScoreAward {
    bool accepted = false;
    bool clean = false;
    std::uint64_t points = 0U;
    std::uint32_t multiplierTenths = 10U;
};

inline std::uint64_t saturatingScoreAdd(std::uint64_t current,
                                        std::uint64_t addition) noexcept {
    if (current >= kScoreCap) return kScoreCap;
    return addition > kScoreCap - current ? kScoreCap : current + addition;
}

inline std::uint64_t baseGateScore(GateKind kind) noexcept {
    switch (kind) {
    case GateKind::Standard: return 100U;
    case GateKind::Precision: return 180U;
    case GateKind::Offset: return 140U;
    case GateKind::Wide: return 80U;
    }
    return 0U;
}

// Called once for each valid forward crossing. Collision and scoring consume
// the same course-relative coordinates.
inline GateScoreAward awardGatePass(RunScore& score,
                                    const ProceduralGate& gate,
                                    float crossingX, float crossingY) noexcept {
    GateScoreAward award;
    if (!std::isfinite(crossingX) || !std::isfinite(crossingY) ||
        !std::isfinite(gate.offsetX) || !std::isfinite(gate.offsetY) ||
        !std::isfinite(gate.apertureRadius)) return award;

    const std::uint64_t base = baseGateScore(gate.kind);
    const float safeRadius = gate.apertureRadius - kCraftCollisionRadius;
    if (base == 0U || safeRadius <= 0.0F) return award;
    const float dx = crossingX - gate.offsetX;
    const float dy = crossingY - gate.offsetY;
    const float radialDistance = std::sqrt(dx * dx + dy * dy);
    if (!std::isfinite(radialDistance) || radialDistance >= safeRadius) return award;

    const float accuracy = std::clamp(1.0F - radialDistance / safeRadius, 0.0F, 1.0F);
    const bool clean = accuracy >= 0.70F;
    const auto comboBonus = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(score.combo, 40U));
    const std::uint32_t multiplierTenths = 10U + comboBonus;
    const std::uint64_t accuracyBonus = static_cast<std::uint64_t>(
        std::lround(static_cast<double>(accuracy) * 100.0));
    const std::uint64_t raw = base + accuracyBonus + (clean ? 75U : 0U);
    const std::uint64_t points = raw * multiplierTenths / 10U;

    score.total = saturatingScoreAdd(score.total, points);
    score.gatesPassed = saturatingScoreAdd(score.gatesPassed, 1U);
    if (clean) score.cleanPasses = saturatingScoreAdd(score.cleanPasses, 1U);
    if (score.combo < kScoreCap) ++score.combo;
    score.bestCombo = std::max(score.bestCombo, score.combo);
    score.lastAward = points;

    award.accepted = true;
    award.clean = clean;
    award.points = points;
    award.multiplierTenths = multiplierTenths;
    return award;
}

inline void breakScoreCombo(RunScore& score) noexcept {
    score.combo = 0U;
}

} // namespace tunrun
