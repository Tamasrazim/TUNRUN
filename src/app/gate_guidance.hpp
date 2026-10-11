#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {

enum class GateApproachStage : std::uint8_t {
    Clear,
    Prepare,
    Approach,
    Imminent
};

struct GateApproachCue {
    bool valid = false;
    float distanceAhead = 0.0F;
    float timeAheadSeconds = 0.0F;
    float urgency = 0.0F;
    GateApproachStage stage = GateApproachStage::Clear;
};

[[nodiscard]] inline const char* gateApproachStageName(
    GateApproachStage stage) noexcept {
    switch (stage) {
    case GateApproachStage::Clear: return "CLEAR";
    case GateApproachStage::Prepare: return "PREPARE";
    case GateApproachStage::Approach: return "APPROACH";
    case GateApproachStage::Imminent: return "IMMINENT";
    }
    return "UNKNOWN";
}

// Describe when the next aperture arrives without revealing scenery beyond it.
// Time-to-gate is derived from the same live forward speed shown in the HUD,
// so acceleration, braking, precision flight and dash all affect the cue.
[[nodiscard]] inline GateApproachCue gateApproachCue(
    double gateDistance, double playerDistance, float forwardSpeed) noexcept {
    if (!std::isfinite(gateDistance) || !std::isfinite(playerDistance) ||
        !std::isfinite(forwardSpeed) || forwardSpeed < 0.0F) {
        return {};
    }

    const double rawDistance = gateDistance - playerDistance;
    if (!std::isfinite(rawDistance)) return {};

    const double nonNegativeDistance = std::max(0.0, rawDistance);
    const double maximumDistance =
        static_cast<double>(std::numeric_limits<float>::max());
    const float distanceAhead = static_cast<float>(
        std::min(nonNegativeDistance, maximumDistance));
    const float timeAhead = distanceAhead / std::max(0.1F, forwardSpeed);
    if (!std::isfinite(timeAhead)) return {};

    GateApproachStage stage = GateApproachStage::Clear;
    if (timeAhead <= 1.0F) {
        stage = GateApproachStage::Imminent;
    } else if (timeAhead <= 2.5F) {
        stage = GateApproachStage::Approach;
    } else if (timeAhead <= 4.0F) {
        stage = GateApproachStage::Prepare;
    }

    const float urgency = std::clamp((4.0F - timeAhead) / 4.0F, 0.0F, 1.0F);
    return GateApproachCue{true, distanceAhead, timeAhead, urgency, stage};
}

} // namespace tunrun
