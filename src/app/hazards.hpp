#pragma once

#include "app/procedural_course.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {

inline constexpr std::uint32_t kHazardGeneratorVersion = 1U;
inline constexpr double kHazardBaseDistance = 88.0;
inline constexpr double kHazardSpacing = 84.0;
inline constexpr double kHazardMinimumGateSeparation = 8.0;
inline constexpr float kHazardMinimumRadius = 0.48F;
inline constexpr float kHazardMaximumRadius = 0.66F;
inline constexpr float kHazardCraftCollisionRadius = 0.42F;

struct ProceduralHazard {
    std::uint32_t index = 0U;
    double distance = 0.0;
    float baseX = 0.0F;
    float baseY = 0.0F;
    float amplitudeX = 0.8F;
    float amplitudeY = 0.4F;
    float frequency = 1.0F;
    float phaseX = 0.0F;
    float phaseY = 0.0F;
    float radius = 0.56F;
};

struct HazardCenter {
    float x = 0.0F;
    float y = 0.0F;
};

struct HazardValidation {
    bool valid = false;
    std::uint32_t hazardsChecked = 0U;
    float maximumHorizontalExtent = 0.0F;
    float maximumVerticalExtent = 0.0F;
    double minimumGateSeparation = std::numeric_limits<double>::infinity();
    const char* failure = "not validated";
};

inline ProceduralHazard hazardAt(std::uint64_t seed,
                                 std::uint32_t index) noexcept {
    const auto sample = static_cast<std::int64_t>(index);
    const double firstDistance = kHazardBaseDistance +
        static_cast<double>(courseUnit(seed, 0, 901U)) * 10.0;
    return ProceduralHazard{
        index,
        firstDistance + static_cast<double>(index) * kHazardSpacing,
        courseSigned(seed, sample, 902U) * 1.20F,
        courseSigned(seed, sample, 903U) * 0.80F,
        0.55F + courseUnit(seed, sample, 904U) * 0.65F,
        0.25F + courseUnit(seed, sample, 905U) * 0.50F,
        0.70F + courseUnit(seed, sample, 906U) * 0.75F,
        courseUnit(seed, sample, 907U) * 2.0F * 3.14159265358979323846F,
        courseUnit(seed, sample, 908U) * 2.0F * 3.14159265358979323846F,
        kHazardMinimumRadius + courseUnit(seed, sample, 909U) *
            (kHazardMaximumRadius - kHazardMinimumRadius)
    };
}

// The mine's motion is a pure function of the run clock. Given the same seed
// and elapsed gameplay time, the center is identical on every run.
inline HazardCenter hazardCenterAt(const ProceduralHazard& hazard,
                                   double elapsedSeconds) noexcept {
    const double xPhase = elapsedSeconds * static_cast<double>(hazard.frequency) +
                          static_cast<double>(hazard.phaseX);
    const double yPhase = elapsedSeconds * static_cast<double>(hazard.frequency) * 0.73 +
                          static_cast<double>(hazard.phaseY);
    return HazardCenter{
        hazard.baseX + hazard.amplitudeX * static_cast<float>(std::sin(xPhase)),
        hazard.baseY + hazard.amplitudeY * static_cast<float>(std::sin(yPhase))
    };
}

inline bool crossesHazardPlane(double previousDistance, double currentDistance,
                               const ProceduralHazard& hazard) noexcept {
    if (!std::isfinite(previousDistance) || !std::isfinite(currentDistance) ||
        !std::isfinite(hazard.distance)) return false;
    return previousDistance <= hazard.distance &&
           currentDistance >= hazard.distance &&
           currentDistance > previousDistance;
}

inline bool collidesWithHazard(float x, float y,
                               const ProceduralHazard& hazard,
                               double elapsedSeconds,
                               float craftRadius = kHazardCraftCollisionRadius) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(hazard.baseX) || !std::isfinite(hazard.baseY) ||
        !std::isfinite(hazard.amplitudeX) || !std::isfinite(hazard.amplitudeY) ||
        !std::isfinite(hazard.frequency) || !std::isfinite(hazard.phaseX) ||
        !std::isfinite(hazard.phaseY) || !std::isfinite(hazard.radius) ||
        !std::isfinite(elapsedSeconds) || !std::isfinite(craftRadius)) return true;
    const HazardCenter center = hazardCenterAt(hazard, elapsedSeconds);
    if (!std::isfinite(center.x) || !std::isfinite(center.y)) return true;
    const float combinedRadius = hazard.radius + std::max(0.0F, craftRadius);
    if (combinedRadius <= 0.0F) return true;
    const float dx = x - center.x;
    const float dy = y - center.y;
    return dx * dx + dy * dy <= combinedRadius * combinedRadius;
}

// Tests the full frame-to-frame relative trajectory, including longitudinal
// separation from the mine. Interpolating the mine center between frame times
// makes this a swept 3D closest-approach test, instead of checking only the
// exact center plane and potentially missing contact at the sphere's near edge.
inline bool sweptCollidesWithHazard(
    float previousX, float previousY, double previousDistance, double previousTime,
    float currentX, float currentY, double currentDistance, double currentTime,
    const ProceduralHazard& hazard,
    float craftRadius = kHazardCraftCollisionRadius) noexcept {
    if (!std::isfinite(previousX) || !std::isfinite(previousY) ||
        !std::isfinite(currentX) || !std::isfinite(currentY) ||
        !std::isfinite(previousDistance) || !std::isfinite(currentDistance) ||
        !std::isfinite(previousTime) || !std::isfinite(currentTime) ||
        !std::isfinite(hazard.distance) || !std::isfinite(hazard.radius) ||
        !std::isfinite(craftRadius)) return true;
    if (currentDistance <= previousDistance || currentTime < previousTime) return false;

    const HazardCenter previousCenter = hazardCenterAt(hazard, previousTime);
    const HazardCenter currentCenter = hazardCenterAt(hazard, currentTime);
    if (!std::isfinite(previousCenter.x) || !std::isfinite(previousCenter.y) ||
        !std::isfinite(currentCenter.x) || !std::isfinite(currentCenter.y)) return true;

    // Relative coordinates include both player and mine movement; course
    // distance supplies the longitudinal axis of the swept segment.
    const double rx0 = static_cast<double>(previousX - previousCenter.x);
    const double ry0 = static_cast<double>(previousY - previousCenter.y);
    const double rz0 = previousDistance - hazard.distance;
    const double rx1 = static_cast<double>(currentX - currentCenter.x);
    const double ry1 = static_cast<double>(currentY - currentCenter.y);
    const double rz1 = currentDistance - hazard.distance;
    const double dx = rx1 - rx0;
    const double dy = ry1 - ry0;
    const double dz = rz1 - rz0;
    const double lengthSquared = dx * dx + dy * dy + dz * dz;
    double fraction = 0.0;
    if (lengthSquared > 1.0e-12) {
        fraction = std::clamp(
            -(rx0 * dx + ry0 * dy + rz0 * dz) / lengthSquared, 0.0, 1.0);
    }
    const double closestX = rx0 + dx * fraction;
    const double closestY = ry0 + dy * fraction;
    const double closestZ = rz0 + dz * fraction;
    const double combinedRadius = static_cast<double>(hazard.radius) +
                                  std::max(0.0F, craftRadius);
    if (combinedRadius <= 0.0) return true;
    return closestX * closestX + closestY * closestY + closestZ * closestZ
           <= combinedRadius * combinedRadius;
}

inline std::uint64_t hazardHash(std::uint64_t seed,
                                std::uint32_t hazardCount = 128U) noexcept {
    if (hazardCount == 0U || hazardCount > 10000U) return 0U;
    std::uint64_t hash = 14695981039346656037ULL;
    const auto absorb = [&hash](std::uint64_t value) {
        for (int byte = 0; byte < 8; ++byte) {
            hash ^= value & 0xFFULL;
            hash *= 1099511628211ULL;
            value >>= 8U;
        }
    };
    absorb(kHazardGeneratorVersion);
    absorb(hazardCount);
    for (std::uint32_t i = 0U; i < hazardCount; ++i) {
        const auto hazard = hazardAt(seed, i);
        absorb(hazard.index);
        absorb(static_cast<std::uint64_t>(std::llround(hazard.distance * 1000.0)));
        absorb(static_cast<std::uint64_t>(static_cast<std::int64_t>(
            std::llround(hazard.baseX * 10000.0F))));
        absorb(static_cast<std::uint64_t>(static_cast<std::int64_t>(
            std::llround(hazard.baseY * 10000.0F))));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.amplitudeX * 10000.0F)));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.amplitudeY * 10000.0F)));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.frequency * 10000.0F)));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.phaseX * 10000.0F)));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.phaseY * 10000.0F)));
        absorb(static_cast<std::uint64_t>(std::llround(hazard.radius * 10000.0F)));
    }
    return hash;
}

inline HazardValidation validateHazardSet(std::uint64_t seed,
                                          std::uint32_t hazardCount = 128U) noexcept {
    HazardValidation result;
    if (hazardCount == 0U || hazardCount > 10000U) {
        result.failure = "invalid hazard count";
        return result;
    }
    double previousDistance = -1.0;
    const double firstGateDistance = gateAt(seed, 0U).distance;
    for (std::uint32_t i = 0U; i < hazardCount; ++i) {
        const auto hazard = hazardAt(seed, i);
        if (hazard.index != i || !std::isfinite(hazard.distance) ||
            hazard.distance <= previousDistance) {
            result.failure = "hazard indices or distances are invalid";
            return result;
        }
        if (!std::isfinite(hazard.baseX) || !std::isfinite(hazard.baseY) ||
            !std::isfinite(hazard.amplitudeX) || !std::isfinite(hazard.amplitudeY) ||
            !std::isfinite(hazard.frequency) || !std::isfinite(hazard.phaseX) ||
            !std::isfinite(hazard.phaseY) || !std::isfinite(hazard.radius)) {
            result.failure = "hazard parameters are non-finite";
            return result;
        }
        if (hazard.amplitudeX < 0.55F || hazard.amplitudeX > 1.20F ||
            hazard.amplitudeY < 0.25F || hazard.amplitudeY > 0.75F ||
            hazard.frequency < 0.70F || hazard.frequency > 1.45F ||
            hazard.radius < kHazardMinimumRadius ||
            hazard.radius > kHazardMaximumRadius) {
            result.failure = "hazard motion or radius outside generator bounds";
            return result;
        }
        const float horizontalExtent = std::abs(hazard.baseX) + hazard.amplitudeX;
        const float verticalExtent = std::abs(hazard.baseY) + hazard.amplitudeY;
        result.maximumHorizontalExtent = std::max(result.maximumHorizontalExtent,
                                                   horizontalExtent);
        result.maximumVerticalExtent = std::max(result.maximumVerticalExtent,
                                                 verticalExtent);
        if (horizontalExtent > 2.401F || verticalExtent > 1.551F) {
            result.failure = "hazard trajectory outside declared bounds";
            return result;
        }

        // Check nearby gates as well as regular spacing. Hazards stay separate
        // from aperture gates instead of stacking two threats in one reaction.
        const auto nearestGate = static_cast<std::int64_t>(std::llround(
            (hazard.distance - firstGateDistance) / kGateSpacing));
        for (std::int64_t offset = -1; offset <= 1; ++offset) {
            const std::int64_t gateIndex = nearestGate + offset;
            if (gateIndex < 0) continue;
            const auto gate = gateAt(seed, static_cast<std::uint32_t>(gateIndex));
            const double separation = std::abs(hazard.distance - gate.distance);
            result.minimumGateSeparation = std::min(result.minimumGateSeparation, separation);
            if (separation < kHazardMinimumGateSeparation) {
                result.failure = "hazard overlaps a gate reaction window";
                return result;
            }
        }
        previousDistance = hazard.distance;
        ++result.hazardsChecked;
    }
    result.valid = true;
    result.failure = "ok";
    return result;
}

} // namespace tunrun
