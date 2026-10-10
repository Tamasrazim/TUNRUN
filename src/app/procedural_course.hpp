#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "app/ship_catalog.hpp"

namespace tunrun {
inline constexpr std::uint32_t kCourseGeneratorVersion = 1U;
inline constexpr double kCourseNodeSpacing = 18.0;
inline constexpr float kCourseMinRadius = 4.65F;
inline constexpr float kCourseMaxRadius = 6.15F;
inline constexpr float kCourseMaxCenterX = 2.10F;
inline constexpr float kCourseMaxCenterY = 1.55F;
inline constexpr float kCourseMaxTwist = 0.58F;
inline constexpr double kGateBaseDistance = 26.0;
inline constexpr double kGateSpacing = 42.0;
inline constexpr float kGateDepthHalfThickness = 0.40F;
inline constexpr float kGateThroatHalfLength = 18.0F;
inline constexpr float kGateThroatNarrowCoreFraction = 0.50F;
inline constexpr float kGateThroatBendAmplitudeX = 1.90F;
inline constexpr float kGateThroatBendAmplitudeY = 0.35F;
inline constexpr float kGateThroatBendMinimumAmplitudeX = 1.45F;
inline constexpr float kGateThroatBendMinimumAmplitudeY = 0.20F;
inline constexpr std::uint64_t kGateThroatBendChannelX = 125U;
inline constexpr std::uint64_t kGateThroatBendChannelY = 126U;
inline constexpr std::uint64_t kGateThroatShapeFamilyChannel = 127U;
inline constexpr float kCraftCollisionRadius = 0.42F;
inline constexpr std::uint32_t kObstacleGeneratorVersion = 6U;
inline constexpr float kGateMinApertureRadius = 1.35F;
inline constexpr float kGateMaxApertureRadius = 2.45F;
inline constexpr float kGateMaxOffsetX = 1.10F;
inline constexpr float kGateMaxOffsetY = 0.80F;

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
enum class GateKind : std::uint8_t {
    Standard,
    Precision,
    Offset,
    Wide
};
struct ProceduralGate {
    std::uint32_t index = 0U;
    double distance = 0.0;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float apertureRadius = 1.85F;
    GateKind kind = GateKind::Standard;
};
struct ObstacleValidation {
    bool valid = false;
    std::uint32_t gatesChecked = 0U;
    std::uint32_t standardGates = 0U;
    std::uint32_t precisionGates = 0U;
    std::uint32_t offsetGates = 0U;
    std::uint32_t wideGates = 0U;
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
        4.75F + courseUnit(seed, node, 3U) * 1.40F,
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
inline const char* gateKindName(GateKind kind) noexcept {
    switch (kind) {
    case GateKind::Standard: return "STANDARD";
    case GateKind::Precision: return "PRECISION";
    case GateKind::Offset: return "OFFSET";
    case GateKind::Wide: return "WIDE";
    }
    return "UNKNOWN";
}

inline ProceduralGate gateAt(std::uint64_t seed, std::uint32_t index) noexcept {
    const auto sample = static_cast<std::int64_t>(index);
    const double firstDistance = kGateBaseDistance +
        static_cast<double>(courseUnit(seed, 0, 111U)) * 4.0;
    const float kindRoll = courseUnit(seed, sample, 124U);
    GateKind kind = GateKind::Standard;
    float radiusMin = 1.75F, radiusMax = 2.05F;
    float offsetScaleX = 0.70F, offsetScaleY = 0.50F;
    if (kindRoll < 0.30F) {
        kind = GateKind::Precision;
        radiusMin = 1.35F; radiusMax = 1.55F;
        offsetScaleX = 0.36F; offsetScaleY = 0.28F;
    } else if (kindRoll < 0.46F) {
        kind = GateKind::Wide;
        radiusMin = 2.20F; radiusMax = 2.45F;
        offsetScaleX = 0.25F; offsetScaleY = 0.22F;
    } else if (kindRoll < 0.76F) {
        kind = GateKind::Offset;
        radiusMin = 1.70F; radiusMax = 1.95F;
        offsetScaleX = 1.10F; offsetScaleY = 0.80F;
    }
    return ProceduralGate{
        index,
        firstDistance + static_cast<double>(index) * kGateSpacing,
        courseSigned(seed, sample, 121U) * offsetScaleX,
        courseSigned(seed, sample, 122U) * offsetScaleY,
        radiusMin + courseUnit(seed, sample, 123U) * (radiusMax - radiusMin),
        kind
    };
}

enum class GateThroatShapeFamily : std::uint8_t {
    SingleS,
    DoubleS,
    Helical,
    SplitWave
};

[[nodiscard]] inline GateThroatShapeFamily gateThroatShapeFamilyAt(
    std::uint64_t seed, std::uint32_t gateIndex) noexcept {
    const float roll = courseUnit(seed, static_cast<std::int64_t>(gateIndex),
                                  kGateThroatShapeFamilyChannel);
    if (roll < 0.25F) return GateThroatShapeFamily::SingleS;
    if (roll < 0.50F) return GateThroatShapeFamily::DoubleS;
    if (roll < 0.75F) return GateThroatShapeFamily::Helical;
    return GateThroatShapeFamily::SplitWave;
}

[[nodiscard]] inline bool validGateThroatShapeFamily(
    GateThroatShapeFamily family) noexcept {
    switch (family) {
    case GateThroatShapeFamily::SingleS:
    case GateThroatShapeFamily::DoubleS:
    case GateThroatShapeFamily::Helical:
    case GateThroatShapeFamily::SplitWave:
        return true;
    }
    return false;
}

[[nodiscard]] inline const char* gateThroatShapeFamilyName(
    GateThroatShapeFamily family) noexcept {
    switch (family) {
    case GateThroatShapeFamily::SingleS: return "S-BEND";
    case GateThroatShapeFamily::DoubleS: return "DOUBLE S";
    case GateThroatShapeFamily::Helical: return "HELICAL WEAVE";
    case GateThroatShapeFamily::SplitWave: return "SPLIT WAVE";
    }
    return "UNKNOWN THROAT";
}

// Every channel is independent of aperture type and visual support geometry.
// All wave combinations are convex blends of sine waves, so the unit envelope
// is preserved even though neighbouring gates now have different topologies.
struct GateThroatBendProfile {
    float amplitudeX = kGateThroatBendAmplitudeX;
    float amplitudeY = kGateThroatBendAmplitudeY;
    GateThroatShapeFamily family = GateThroatShapeFamily::SingleS;
};

[[nodiscard]] inline GateThroatBendProfile gateThroatBendProfileAt(
    std::uint64_t seed, std::uint32_t gateIndex) noexcept {
    const auto index = static_cast<std::int64_t>(gateIndex);
    return GateThroatBendProfile{
        kGateThroatBendMinimumAmplitudeX +
            courseUnit(seed, index, kGateThroatBendChannelX) *
                (kGateThroatBendAmplitudeX - kGateThroatBendMinimumAmplitudeX),
        kGateThroatBendMinimumAmplitudeY +
            courseUnit(seed, index, kGateThroatBendChannelY) *
                (kGateThroatBendAmplitudeY - kGateThroatBendMinimumAmplitudeY),
        gateThroatShapeFamilyAt(seed, gateIndex)
    };
}

struct GateThroatBendOffset {
    float x = 0.0F;
    float y = 0.0F;
};

[[nodiscard]] inline GateThroatBendOffset gateThroatBendOffsetAt(
    std::uint64_t seed, std::uint32_t gateIndex, double throatOffset) noexcept {
    if (!std::isfinite(throatOffset)) return {};
    const auto profile = gateThroatBendProfileAt(seed, gateIndex);
    const double t = std::clamp(
        throatOffset / static_cast<double>(kGateThroatHalfLength), -1.0, 1.0);
    constexpr double pi = 3.14159265358979323846;
    const float single = static_cast<float>(std::sin(pi * t));
    const float doubleWave = static_cast<float>(std::sin(2.0 * pi * t));
    const float triple = static_cast<float>(std::sin(3.0 * pi * t));
    float horizontal = single;
    float vertical = doubleWave;
    switch (profile.family) {
    case GateThroatShapeFamily::SingleS:
        break;
    case GateThroatShapeFamily::DoubleS:
        horizontal = doubleWave;
        vertical = single;
        break;
    case GateThroatShapeFamily::Helical:
        horizontal = 0.72F * single + 0.28F * doubleWave;
        vertical = 0.50F * doubleWave + 0.50F * triple;
        break;
    case GateThroatShapeFamily::SplitWave:
        horizontal = 0.62F * single + 0.38F * triple;
        vertical = 0.38F * single + 0.62F * doubleWave;
        break;
    }
    const std::uint64_t turnBits = mixCourseBits(
        seed ^ (static_cast<std::uint64_t>(gateIndex) * 0x9E3779B97F4A7C15ULL));
    const float turnSign = (turnBits & 1ULL) != 0ULL ? 1.0F : -1.0F;
    return GateThroatBendOffset{
        turnSign * profile.amplitudeX * horizontal,
        profile.amplitudeY * vertical
    };
}

// Shared gate-passage geometry. Rendering, camera clearance, route guidance,
// and collision all use this same taper so the opening is a real corridor.
struct GateThroatSection {
    bool active = false;
    std::uint32_t gateIndex = 0U;
    float centerX = 0.0F;
    float centerY = 0.0F;
    float radius = 5.75F;
    float pinch = 0.0F;
};

[[nodiscard]] inline GateThroatSection gateThroatSectionAtDistance(
    std::uint64_t seed, const ProceduralGate& gate, double distance) noexcept {
    if (!std::isfinite(distance) || !std::isfinite(gate.distance)) {
        return GateThroatSection{false, gate.index, 0.0F, 0.0F,
                                 kCourseMinRadius, 0.0F};
    }
    distance = std::clamp(distance, -18000000.0, 18000000.0);
    const auto course = sampleCourse(seed, distance);
    const double offset = distance - gate.distance;
    const double absoluteOffset = std::abs(offset);
    if (absoluteOffset >= static_cast<double>(kGateThroatHalfLength)) {
        return GateThroatSection{false, gate.index, 0.0F, 0.0F,
                                 course.radius, 0.0F};
    }
    // Keep the minimum aperture for half the sleeve length on each side of
    // the plane, then flare into the main tunnel. This creates a genuinely
    // deep constriction rather than a thin ring that reveals the next section.
    const float normalizedDistance = static_cast<float>(
        absoluteOffset / static_cast<double>(kGateThroatHalfLength));
    const float taperProgress = std::clamp(
        (normalizedDistance - kGateThroatNarrowCoreFraction) /
            (1.0F - kGateThroatNarrowCoreFraction),
        0.0F, 1.0F);
    const float taper = taperProgress * taperProgress * (3.0F - 2.0F * taperProgress);
    const float pinch = 1.0F - taper;
    const float safeRadius = std::max(0.8F, course.radius - 0.16F);
    const float aperture = std::clamp(gate.apertureRadius, 0.8F, safeRadius);

    // Four deterministic waveform topologies change the actual corridor:
    // single S, double S, blended helical weave, and split wave. All systems
    // share this helper through the sampled throat centerline.
    const auto bend = gateThroatBendOffsetAt(seed, gate.index, offset);
    const float bendX = bend.x;
    const float bendY = bend.y;
    return GateThroatSection{
        true, gate.index, gate.offsetX * pinch + bendX,
        gate.offsetY * pinch + bendY,
        course.radius - (course.radius - aperture) * pinch, pinch
    };
}

[[nodiscard]] inline GateThroatSection gateThroatSectionAtDistance(
    std::uint64_t seed, double distance) noexcept {
    if (!std::isfinite(distance)) {
        return GateThroatSection{false, 0U, 0.0F, 0.0F,
                                 kCourseMinRadius, 0.0F};
    }
    distance = std::clamp(distance, -18000000.0, 18000000.0);
    const auto first = gateAt(seed, 0U);
    const auto slot = static_cast<std::int64_t>(
        std::floor((distance - first.distance) / kGateSpacing));
    const std::uint32_t start = slot <= 0 ? 0U
        : static_cast<std::uint32_t>(std::min<std::int64_t>(slot, 1000000));
    for (std::uint32_t i = start; i <= start + 1U; ++i) {
        const auto throat = gateThroatSectionAtDistance(seed, gateAt(seed, i), distance);
        if (throat.active) return throat;
    }
    return GateThroatSection{false, start, 0.0F, 0.0F,
                             sampleCourse(seed, distance).radius, 0.0F};
}

// Give the player a deterministic steering cue into the next narrow sleeve.
// While still inside a gate's trailing throat half, keep guiding toward that
// throat rather than jumping immediately to the next gate after its plane.
struct GateThroatGuidance {
    bool valid = false;
    std::uint32_t gateIndex = 0U;
    float targetX = 0.0F;
    float targetY = 0.0F;
    float lateralError = 0.0F;
    float verticalError = 0.0F;
    float distanceAhead = 0.0F;
    float targetRadius = 0.0F;
};

[[nodiscard]] inline GateThroatGuidance gateThroatGuidanceForFlight(
    std::uint64_t seed, double playerDistance, float playerX, float playerY,
    float actualForwardSpeed) noexcept {
    if (!std::isfinite(playerDistance) || !std::isfinite(playerX) ||
        !std::isfinite(playerY)) return {};
    playerDistance = std::clamp(playerDistance, -18000000.0, 18000000.0);
    if (!std::isfinite(actualForwardSpeed)) actualForwardSpeed = 11.0F;
    const double lookAhead = static_cast<double>(
        std::clamp(actualForwardSpeed * 0.82F, 3.0F, 13.0F));
    const auto firstGate = gateAt(seed, 0U);
    const double relativeGateSlot =
        (playerDistance - firstGate.distance) / kGateSpacing;
    const auto slot = static_cast<std::int64_t>(std::floor(relativeGateSlot));
    const std::uint32_t boundedSlot = slot <= 0 ? 0U
        : static_cast<std::uint32_t>(std::min<std::int64_t>(slot, 1000000));
    auto gate = gateAt(seed, boundedSlot);
    if (playerDistance > gate.distance &&
        !gateThroatSectionAtDistance(seed, gate, playerDistance).active) {
        gate = gateAt(seed, boundedSlot + 1U);
    }

    const double approachStart = gate.distance - 12.0;
    const double desiredDistance = std::max(playerDistance + lookAhead, approachStart);
    const double guideDistance = std::clamp(
        desiredDistance, approachStart,
        gate.distance + static_cast<double>(kGateThroatHalfLength) - 0.5);
    const auto throat = gateThroatSectionAtDistance(seed, gate, guideDistance);
    if (!throat.active) return {};
    const auto currentCourse = sampleCourse(seed, playerDistance);
    const auto guideCourse = sampleCourse(seed, guideDistance);
    const float targetX = throat.centerX +
        (guideCourse.centerX - currentCourse.centerX);
    const float targetY = throat.centerY +
        (guideCourse.centerY - currentCourse.centerY);
    return GateThroatGuidance{
        true, gate.index, targetX, targetY, targetX - playerX, targetY - playerY,
        static_cast<float>(std::max(0.0, guideDistance - playerDistance)),
        throat.radius
    };
}

[[nodiscard]] inline bool collidesWithGateThroatAtDistance(
    float x, float y, std::uint64_t seed, const ProceduralGate& gate,
    double distance, float craftRadius = kCraftCollisionRadius) noexcept {
    const auto throat = gateThroatSectionAtDistance(seed, gate, distance);
    if (!throat.active) return false;
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(craftRadius)) return true;
    const float safeRadius = throat.radius - std::max(0.0F, craftRadius);
    if (safeRadius <= 0.0F) return true;
    const float dx = x - throat.centerX;
    const float dy = y - throat.centerY;
    return dx * dx + dy * dy >= safeRadius * safeRadius;
}

[[nodiscard]] inline bool collidesWithGateThroatAlongSegment(
    std::uint64_t seed, const ProceduralGate& gate,
    float previousX, float previousY, double previousDistance,
    float currentX, float currentY, double currentDistance,
    float craftRadius = kCraftCollisionRadius) noexcept {
    if (!std::isfinite(previousX) || !std::isfinite(previousY) ||
        !std::isfinite(currentX) || !std::isfinite(currentY) ||
        !std::isfinite(previousDistance) || !std::isfinite(currentDistance)) return true;
    const double distanceSpan = std::abs(currentDistance - previousDistance);
    const auto previousCourse = sampleCourse(seed, previousDistance);
    const auto currentCourse = sampleCourse(seed, currentDistance);
    const double worldX0 = static_cast<double>(previousX) + previousCourse.centerX;
    const double worldY0 = static_cast<double>(previousY) + previousCourse.centerY;
    const double worldX1 = static_cast<double>(currentX) + currentCourse.centerX;
    const double worldY1 = static_cast<double>(currentY) + currentCourse.centerY;
    const double lateralSpan = std::hypot(worldX1 - worldX0, worldY1 - worldY0);
    const double sampleSpan = std::max(distanceSpan, lateralSpan);
    if (!std::isfinite(sampleSpan) || sampleSpan > 256.0) return true;
    const int samples = std::clamp(
        static_cast<int>(std::ceil(sampleSpan / 0.15)), 1, 2048);
    for (int i = 0; i <= samples; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(samples);
        const double distance = previousDistance +
            (currentDistance - previousDistance) * t;
        // Gate-plane contact uses shared crossing interpolation below. The
        // sleeve handles finite approach/departure length around that plane.
        if (std::abs(distance - gate.distance) <= kGateDepthHalfThickness) continue;
        const auto course = sampleCourse(seed, distance);
        const float x = static_cast<float>(
            worldX0 + (worldX1 - worldX0) * t - course.centerX);
        const float y = static_cast<float>(
            worldY0 + (worldY1 - worldY0) * t - course.centerY);
        if (collidesWithGateThroatAtDistance(x, y, seed, gate, distance, craftRadius)) {
            return true;
        }
    }
    return false;
}

// Canonical identity for obstacle gameplay data, separate from the tunnel
// centerline hash so a gate-rule change cannot silently reuse the same hash.
inline std::uint64_t obstacleHash(std::uint64_t seed,
                                  std::uint32_t gateCount = 128U) noexcept {
    if (gateCount == 0U || gateCount > 10000U) return 0U;
    std::uint64_t hash = 14695981039346656037ULL;
    const auto absorb = [&hash](std::int64_t value) {
        std::uint64_t bits = static_cast<std::uint64_t>(value);
        for (int byte = 0; byte < 8; ++byte) {
            hash ^= bits & 0xFFULL;
            hash *= 1099511628211ULL;
            bits >>= 8U;
        }
    };
    absorb(static_cast<std::int64_t>(kObstacleGeneratorVersion));
    absorb(static_cast<std::int64_t>(gateCount));
    for (std::uint32_t i = 0; i < gateCount; ++i) {
        const auto gate = gateAt(seed, i);
        absorb(static_cast<std::int64_t>(gate.index));
        absorb(static_cast<std::int64_t>(std::llround(gate.distance * 1000.0)));
        absorb(static_cast<std::int64_t>(std::llround(gate.offsetX * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(gate.offsetY * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(gate.apertureRadius * 10000.0F)));
        absorb(static_cast<std::int64_t>(gate.kind));
        const auto throatShape = gateThroatBendProfileAt(seed, gate.index);
        absorb(static_cast<std::int64_t>(std::llround(throatShape.amplitudeX * 10000.0F)));
        absorb(static_cast<std::int64_t>(std::llround(throatShape.amplitudeY * 10000.0F)));
        absorb(static_cast<std::int64_t>(throatShape.family));
    }
    return hash;
}
inline bool collidesWithGate(float x, float y, const ProceduralGate& gate,
                             float craftRadius = kCraftCollisionRadius) noexcept {
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
    // Count only a forward crossing of the gate's centre plane; otherwise a
    // craft lingering inside the gate's thickness could score it repeatedly.
    return previousDistance <= gate.distance && currentDistance >= gate.distance &&
           currentDistance > previousDistance;
}

struct GateCrossingPoint {
    bool crossedPlane = false;
    bool valid = false;
    float x = 0.0F;
    float y = 0.0F;
};

inline GateCrossingPoint gatePointAtCourseCrossing(
    std::uint64_t seed, float previousX, float previousY,
    double previousDistance, float currentX, float currentY,
    double currentDistance, const ProceduralGate& gate) noexcept {
    GateCrossingPoint point;
    if (!std::isfinite(previousDistance) || !std::isfinite(currentDistance) ||
        !std::isfinite(gate.distance)) {
        point.crossedPlane = true;
        return point;
    }
    if (!crossesGatePlane(previousDistance, currentDistance, gate)) return point;
    point.crossedPlane = true;
    if (!std::isfinite(previousX) || !std::isfinite(previousY) ||
        !std::isfinite(currentX) || !std::isfinite(currentY)) return point;
    const double travel = currentDistance - previousDistance;
    if (!std::isfinite(travel) || travel <= 1.0e-9) return point;
    const double fraction = std::clamp(
        (gate.distance - previousDistance) / travel, 0.0, 1.0);
    const auto previousSection = sampleCourse(seed, previousDistance);
    const auto currentSection = sampleCourse(seed, currentDistance);
    const auto gateSection = sampleCourse(seed, gate.distance);
    const double worldX0 = static_cast<double>(previousX) + previousSection.centerX;
    const double worldY0 = static_cast<double>(previousY) + previousSection.centerY;
    const double worldX1 = static_cast<double>(currentX) + currentSection.centerX;
    const double worldY1 = static_cast<double>(currentY) + currentSection.centerY;
    const double relativeX = worldX0 + (worldX1 - worldX0) * fraction -
                             gateSection.centerX;
    const double relativeY = worldY0 + (worldY1 - worldY0) * fraction -
                             gateSection.centerY;
    if (!std::isfinite(relativeX) || !std::isfinite(relativeY)) return point;
    point.x = static_cast<float>(relativeX);
    point.y = static_cast<float>(relativeY);
    point.valid = std::isfinite(point.x) && std::isfinite(point.y);
    return point;
}

// Collision and scoring use the identical crossing point. An invalid point
// after a reported forward crossing fails closed; a non-crossing is harmless.
inline bool collidesWithGateAtCrossingPoint(
    const GateCrossingPoint& point, const ProceduralGate& gate,
    float craftRadius = kCraftCollisionRadius) noexcept {
    if (!point.crossedPlane) return false;
    if (!point.valid) return true;
    return collidesWithGate(point.x, point.y, gate, craftRadius);
}

// Compatibility wrapper for callers that provide the swept flight segment.
// The main loop computes the point itself so it can share it with scoring.
inline bool collidesWithGateAtCourseCrossing(
    std::uint64_t seed, float previousX, float previousY,
    double previousDistance, float currentX, float currentY,
    double currentDistance, const ProceduralGate& gate,
    float craftRadius = kCraftCollisionRadius) noexcept {
    const auto point = gatePointAtCourseCrossing(
        seed, previousX, previousY, previousDistance,
        currentX, currentY, currentDistance, gate);
    return collidesWithGateAtCrossingPoint(point, gate, craftRadius);
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
        if (gate.index != i || !std::isfinite(gate.offsetX) ||
            !std::isfinite(gate.offsetY) || !std::isfinite(gate.apertureRadius)) {
            result.failure = "gate index or parameters are invalid";
            return result;
        }
        float radiusMin = 0.0F, radiusMax = 0.0F;
        switch (gate.kind) {
        case GateKind::Standard:
            radiusMin = 1.75F; radiusMax = 2.05F; ++result.standardGates; break;
        case GateKind::Precision:
            radiusMin = 1.35F; radiusMax = 1.55F; ++result.precisionGates; break;
        case GateKind::Offset:
            radiusMin = 1.70F; radiusMax = 1.95F; ++result.offsetGates; break;
        case GateKind::Wide:
            radiusMin = 2.20F; radiusMax = 2.45F; ++result.wideGates; break;
        default:
            result.failure = "unknown gate kind";
            return result;
        }
        if (gate.apertureRadius < radiusMin || gate.apertureRadius > radiusMax) {
            result.failure = "gate aperture does not match its kind";
            return result;
        }
        const auto throatShape = gateThroatBendProfileAt(seed, gate.index);
        if (!validGateThroatShapeFamily(throatShape.family) ||
            !std::isfinite(throatShape.amplitudeX) ||
            !std::isfinite(throatShape.amplitudeY) ||
            throatShape.amplitudeX < kGateThroatBendMinimumAmplitudeX ||
            throatShape.amplitudeX > kGateThroatBendAmplitudeX ||
            throatShape.amplitudeY < kGateThroatBendMinimumAmplitudeY ||
            throatShape.amplitudeY > kGateThroatBendAmplitudeY) {
            result.failure = "gate throat bend profile outside bounds";
            return result;
        }
        if (std::abs(gate.offsetX) > kGateMaxOffsetX ||
            std::abs(gate.offsetY) > kGateMaxOffsetY) {
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

struct GateTransitionReachability {
    bool valid = false;
    double travelTime = 0.0;
    float requiredShift = 0.0F;
    float reachableShift = 0.0F;
    float previousApertureAllowance = 0.0F;
    float nextApertureAllowance = 0.0F;
    float slack = -std::numeric_limits<float>::infinity();
    const char* failure = "not validated";
};

struct ReachabilityValidation {
    bool valid = false;
    std::uint32_t gatesChecked = 0U;
    std::uint32_t transitionsChecked = 0U;
    std::uint32_t worstTransitionIndex = 0U;
    float maximumRequiredShift = 0.0F;
    float minimumReachableSlack = std::numeric_limits<float>::infinity();
    float worstTransitionReachableShift = 0.0F;
    float worstTransitionAllowance = 0.0F;
    double minimumTravelTime = std::numeric_limits<double>::infinity();
    const char* failure = "not validated";
};

// Screens each pair at the fastest forward pace (continuous boost), using the
// ship's lateral speed and acceleration. Both aperture radii count as endpoint
// tolerance. This is a conservative pairwise screen, not a proof for every
// possible incoming velocity or a complete route-state simulation.
inline GateTransitionReachability evaluateGateTransitionReachability(
    const ProceduralGate& previous, const ProceduralGate& next,
    std::uint32_t shipId) noexcept {
    GateTransitionReachability result;
    if (shipId >= kShipCatalog.size()) {
        result.failure = "unknown ship profile";
        return result;
    }
    if (!std::isfinite(previous.distance) || !std::isfinite(next.distance) ||
        !std::isfinite(previous.offsetX) || !std::isfinite(previous.offsetY) ||
        !std::isfinite(next.offsetX) || !std::isfinite(next.offsetY) ||
        !std::isfinite(previous.apertureRadius) || !std::isfinite(next.apertureRadius)) {
        result.failure = "non-finite gate parameter";
        return result;
    }

    const double gapDistance = next.distance - previous.distance;
    if (gapDistance < kGateSpacing - 0.0001) {
        result.failure = "gate spacing is below the reaction budget";
        return result;
    }
    result.previousApertureAllowance = previous.apertureRadius - kCraftCollisionRadius;
    result.nextApertureAllowance = next.apertureRadius - kCraftCollisionRadius;
    if (result.previousApertureAllowance <= 0.0F ||
        result.nextApertureAllowance <= 0.0F) {
        result.failure = "gate opening is smaller than the craft collision radius";
        return result;
    }

    const auto& ship = shipDefinition(shipId);
    const double maximumForwardSpeed = 16.0 * static_cast<double>(ship.speedMultiplier);
    const double maximumLateralSpeed = 6.0 * static_cast<double>(ship.speedMultiplier);
    const double lateralAcceleration = 10.0 * static_cast<double>(ship.accelerationMultiplier);
    if (!std::isfinite(maximumForwardSpeed) || maximumForwardSpeed <= 0.0 ||
        !std::isfinite(maximumLateralSpeed) || maximumLateralSpeed <= 0.0 ||
        !std::isfinite(lateralAcceleration) || lateralAcceleration <= 0.0) {
        result.failure = "ship movement profile is invalid";
        return result;
    }

    result.travelTime = gapDistance / maximumForwardSpeed;
    const double accelerationTime = maximumLateralSpeed / lateralAcceleration;
    const double reachable = result.travelTime <= accelerationTime
        ? 0.5 * lateralAcceleration * result.travelTime * result.travelTime
        : 0.5 * lateralAcceleration * accelerationTime * accelerationTime +
          maximumLateralSpeed * (result.travelTime - accelerationTime);
    const double dx = static_cast<double>(next.offsetX) - previous.offsetX;
    const double dy = static_cast<double>(next.offsetY) - previous.offsetY;
    const double requiredShift = std::sqrt(dx * dx + dy * dy);
    const double totalAllowance = reachable +
        result.previousApertureAllowance + result.nextApertureAllowance;
    if (!std::isfinite(result.travelTime) || !std::isfinite(reachable) ||
        !std::isfinite(requiredShift) || !std::isfinite(totalAllowance)) {
        result.failure = "non-finite reachability calculation";
        return result;
    }

    result.requiredShift = static_cast<float>(requiredShift);
    result.reachableShift = static_cast<float>(reachable);
    result.slack = static_cast<float>(totalAllowance - requiredShift);
    if (result.slack < -0.0001F) {
        result.failure = "gate transition exceeds the ship reachable envelope";
        return result;
    }
    result.valid = true;
    result.failure = "ok";
    return result;
}

inline ReachabilityValidation validateGateReachability(
    std::uint64_t seed, std::uint32_t gateCount = 128U,
    std::uint32_t shipId = kStarterShipId) noexcept {
    ReachabilityValidation result;
    if (gateCount == 0U || gateCount > 10000U) {
        result.failure = "invalid gate count";
        return result;
    }
    if (shipId >= kShipCatalog.size()) {
        result.failure = "unknown ship profile";
        return result;
    }
    const auto obstacles = validateObstacleSet(seed, gateCount);
    if (!obstacles.valid) {
        result.failure = obstacles.failure;
        return result;
    }

    result.gatesChecked = 1U;
    auto previous = gateAt(seed, 0U);
    for (std::uint32_t i = 1U; i < gateCount; ++i) {
        const auto next = gateAt(seed, i);
        const auto transition = evaluateGateTransitionReachability(previous, next, shipId);
        if (!transition.valid) {
            result.worstTransitionIndex = i;
            result.maximumRequiredShift = std::max(
                result.maximumRequiredShift, transition.requiredShift);
            result.failure = transition.failure;
            return result;
        }
        ++result.transitionsChecked;
        ++result.gatesChecked;
        result.maximumRequiredShift = std::max(
            result.maximumRequiredShift, transition.requiredShift);
        if (transition.slack < result.minimumReachableSlack) {
            result.minimumReachableSlack = transition.slack;
            result.worstTransitionIndex = i;
            result.worstTransitionReachableShift = transition.reachableShift;
            result.worstTransitionAllowance =
                transition.previousApertureAllowance + transition.nextApertureAllowance;
        }
        result.minimumTravelTime = std::min(result.minimumTravelTime, transition.travelTime);
        previous = next;
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
