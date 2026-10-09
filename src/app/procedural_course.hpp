#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {
inline constexpr std::uint32_t kCourseGeneratorVersion = 1U;
inline constexpr double kCourseNodeSpacing = 18.0;
inline constexpr float kCourseMinRadius = 5.35F;
inline constexpr float kCourseMaxRadius = 6.15F;
inline constexpr float kCourseMaxCenterX = 1.45F;
inline constexpr float kCourseMaxCenterY = 1.05F;
inline constexpr float kCourseMaxTwist = 0.38F;
inline constexpr double kGateBaseDistance = 26.0;
inline constexpr double kGateSpacing = 42.0;
inline constexpr float kGateDepthHalfThickness = 0.40F;
inline constexpr float kGateMinApertureRadius = 1.75F;
inline constexpr float kGateMaxApertureRadius = 2.05F;

struct TunnelCrossSection {
    float centerX = 0.0F;
    float centerY = 0.0F;
    float radius = 5.75F;
    float twist = 0.0F;
};
struct CourseValidation {
    bool valid = false;
    std::uint32_t samplesChecked = 0U;
    float minimumRadius = std::numeric_limits<float>::infinity();
    float maximumRadius = 0.0F;
    float maximumCenterOffset = 0.0F;
    const char* failure = "not validated";
};
struct ProceduralGate {
    std::uint32_t index = 0U;
    double distance = 0.0;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float apertureRadius = 1.85F;
};
struct ObstacleValidation {
    bool valid = false;
    std::uint32_t gatesChecked = 0U;
    const char* failure = "not validated";
};

inline std::uint64_t mixCourseBits(std::uint64_t value) noexcept {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}
inline float courseUnit(std::uint64_t seed, std::int64_t node,
                        std::uint64_t channel) noexcept {
    const auto nodeBits = static_cast<std::uint64_t>(node);
    const std::uint64_t value = mixCourseBits(
        seed ^ mixCourseBits(nodeBits * 0xD6E8FEB86659FD93ULL +
                             channel * 0xA0761D6478BD642FULL));
    return static_cast<float>(static_cast<std::uint32_t>(value >> 40U)) *
           (1.0F / 16777215.0F);
}
inline float courseSigned(std::uint64_t seed, std::int64_t node,
                          std::uint64_t channel) noexcept {
    return courseUnit(seed, node, channel) * 2.0F - 1.0F;
}
inline TunnelCrossSection courseControlNode(std::uint64_t seed,
                                            std::int64_t node) noexcept {
    return TunnelCrossSection{
        courseSigned(seed, node, 1U) * kCourseMaxCenterX,
        courseSigned(seed, node, 2U) * kCourseMaxCenterY,
        5.55F + courseUnit(seed, node, 3U) * 0.55F,
        courseSigned(seed, node, 4U) * kCourseMaxTwist
    };
}
inline float courseCatmullRom(float p0, float p1, float p2, float p3,
                              float t) noexcept {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5F * ((2.0F * p1) + (-p0 + p2) * t +
        (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3) * t2 +
        (-p0 + 3.0F * p1 - 3.0F * p2 + p3) * t3);
}
inline TunnelCrossSection sampleCourse(std::uint64_t seed,
                                       double distance) noexcept {
    if (!std::isfinite(distance)) distance = 0.0;
    distance = std::clamp(distance, -18000000.0, 18000000.0);
    const double nodePosition = distance / kCourseNodeSpacing;
    const auto index = static_cast<std::int64_t>(std::floor(nodePosition));
    const float t = static_cast<float>(nodePosition - static_cast<double>(index));
    const auto a = courseControlNode(seed, index - 1);
    const auto b = courseControlNode(seed, index);
    const auto c = courseControlNode(seed, index + 1);
    const auto d = courseControlNode(seed, index + 2);
    return TunnelCrossSection{
        std::clamp(courseCatmullRom(a.centerX,b.centerX,c.centerX,d.centerX,t),
                   -kCourseMaxCenterX,kCourseMaxCenterX),
        std::clamp(courseCatmullRom(a.centerY,b.centerY,c.centerY,d.centerY,t),
                   -kCourseMaxCenterY,kCourseMaxCenterY),
        std::clamp(courseCatmullRom(a.radius,b.radius,c.radius,d.radius,t),
                   kCourseMinRadius,kCourseMaxRadius),
        std::clamp(courseCatmullRom(a.twist,b.twist,c.twist,d.twist,t),
                   -kCourseMaxTwist,kCourseMaxTwist)
    };
}
inline std::uint64_t deriveCourseSeed(std::uint64_t rootSeed,
                                      std::uint64_t runIndex) noexcept {
    return mixCourseBits(rootSeed ^ mixCourseBits(runIndex + 0xD1B54A32D192ED03ULL));
}
inline std::uint64_t courseHash(std::uint64_t seed,
                                std::uint32_t sampleCount = 256U) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    const auto absorb = [&hash](std::int64_t value) {
        std::uint64_t bits = static_cast<std::uint64_t>(value);
        for (int byte = 0; byte < 8; ++byte) {
            hash ^= bits & 0xFFULL;
            hash *= 1099511628211ULL;
            bits >>= 8U;
        }
    };
    absorb(static_cast<std::int64_t>(kCourseGeneratorVersion));
    for (std::uint32_t i = 0; i < sampleCount; ++i) {
        const auto sample = sampleCourse(seed, static_cast<double>(i) * 0.75);
        absorb(static_cast<std::int64_t>(std::llround(sample.centerX * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(sample.centerY * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(sample.radius * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(sample.twist * 10000.0F)));
    }
    return hash;
}
inline ProceduralGate gateAt(std::uint64_t seed, std::uint32_t index) noexcept {
    const double firstDistance = kGateBaseDistance +
        static_cast<double>(courseUnit(seed, 0, 111U)) * 4.0;
    return ProceduralGate{
        index,
        firstDistance + static_cast<double>(index) * kGateSpacing,
        courseSigned(seed, static_cast<std::int64_t>(index), 121U) * 0.95F,
        courseSigned(seed, static_cast<std::int64_t>(index), 122U) * 0.75F,
        kGateMinApertureRadius +
            courseUnit(seed, static_cast<std::int64_t>(index), 123U) *
            (kGateMaxApertureRadius - kGateMinApertureRadius)
    };
}
inline bool collidesWithGate(float x, float y, const ProceduralGate& gate,
                             float craftRadius = 0.42F) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(gate.offsetX) || !std::isfinite(gate.offsetY) ||
        !std::isfinite(gate.apertureRadius) || !std::isfinite(craftRadius)) return true;
    const float safeRadius = gate.apertureRadius - std::max(0.0F, craftRadius);
    if (safeRadius <= 0.0F) return true;
    const float dx = x - gate.offsetX;
    const float dy = y - gate.offsetY;
    return dx * dx + dy * dy >= safeRadius * safeRadius;
}
inline bool crossesGatePlane(double previousDistance, double currentDistance,
                             const ProceduralGate& gate) noexcept {
    if (!std::isfinite(previousDistance) || !std::isfinite(currentDistance)) return true;
    if (currentDistance < previousDistance) std::swap(previousDistance, currentDistance);
    return previousDistance <= gate.distance + kGateDepthHalfThickness &&
           currentDistance >= gate.distance - kGateDepthHalfThickness;
}
inline ObstacleValidation validateObstacleSet(std::uint64_t seed,
                                               std::uint32_t gateCount = 128U) noexcept {
    ObstacleValidation result;
    if (gateCount == 0U || gateCount > 10000U) {
        result.failure = "invalid gate count";
        return result;
    }
    double previousDistance = -1.0;
    for (std::uint32_t i = 0; i < gateCount; ++i) {
        const auto gate = gateAt(seed, i);
        if (!std::isfinite(gate.distance) || gate.distance <= previousDistance) {
            result.failure = "gate distances are not strictly increasing";
            return result;
        }
        if (gate.index != i || gate.apertureRadius < kGateMinApertureRadius ||
            gate.apertureRadius > kGateMaxApertureRadius) {
            result.failure = "gate index/aperture outside bounds";
            return result;
        }
        if (std::abs(gate.offsetX) > 0.95F || std::abs(gate.offsetY) > 0.75F) {
            result.failure = "gate aperture offset outside bounds";
            return result;
        }
        const float requiredRadius = std::sqrt(
            gate.offsetX * gate.offsetX + gate.offsetY * gate.offsetY) +
            gate.apertureRadius + 0.42F;
        if (requiredRadius >= kCourseMinRadius) {
            result.failure = "gate opening leaves nominal tunnel clearance";
            return result;
        }
        if (i > 0U && gate.distance - previousDistance < kGateSpacing - 0.0001) {
            result.failure = "gates overlap their reaction-distance budget";
            return result;
        }
        previousDistance = gate.distance;
        ++result.gatesChecked;
    }
    result.valid = true;
    result.failure = "ok";
    return result;
}

inline CourseValidation validateCourse(std::uint64_t seed, double length,
                                       double requestedStep = 0.75) noexcept {
    CourseValidation result;
    if (!std::isfinite(length) || !std::isfinite(requestedStep) ||
        length <= 0.0 || requestedStep <= 0.0 || length > 100000.0) {
        result.failure = "invalid validation range";
        return result;
    }
    const auto steps = static_cast<std::uint64_t>(std::ceil(length / requestedStep));
    if (steps == 0U || steps > 150000U) {
        result.failure = "validation sample budget exceeded";
        return result;
    }
    for (std::uint64_t i = 0; i <= steps; ++i) {
        const double distance = length * static_cast<double>(i) / static_cast<double>(steps);
        const auto sample = sampleCourse(seed, distance);
        if (!std::isfinite(sample.centerX) || !std::isfinite(sample.centerY) ||
            !std::isfinite(sample.radius) || !std::isfinite(sample.twist)) {
            result.failure = "non-finite course parameter";
            return result;
        }
        if (sample.radius < kCourseMinRadius || sample.radius > kCourseMaxRadius) {
            result.failure = "radius outside generator bounds";
            return result;
        }
        if (std::abs(sample.centerX) > kCourseMaxCenterX ||
            std::abs(sample.centerY) > kCourseMaxCenterY) {
            result.failure = "centerline outside generator bounds";
            return result;
        }
        if (std::abs(sample.twist) > kCourseMaxTwist) {
            result.failure = "twist outside generator bounds";
            return result;
        }
        result.minimumRadius = std::min(result.minimumRadius, sample.radius);
        result.maximumRadius = std::max(result.maximumRadius, sample.radius);
        result.maximumCenterOffset = std::max(result.maximumCenterOffset,
            std::sqrt(sample.centerX * sample.centerX + sample.centerY * sample.centerY));
        ++result.samplesChecked;
    }
    result.valid = true;
    result.failure = "ok";
    return result;
}
} // namespace tunrun
