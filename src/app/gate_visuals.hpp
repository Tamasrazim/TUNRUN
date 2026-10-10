#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "app/procedural_course.hpp"

namespace tunrun {

// Decorative gate architecture is generated independently from gate kind,
// aperture size, collision checks, and route scoring.
inline constexpr std::uint32_t kGateStructureVisualGeneratorVersion = 1U;
inline constexpr std::uint64_t kGateStructureVisualChannel = 191U;
inline constexpr float kGateStructureMinimumApertureGap = 0.18F;
inline constexpr std::size_t kGateStructureFamilyCount = 5U;

enum class GateStructureFamily : std::uint8_t {
    RadialCage,
    SegmentedCrown,
    ChevronBrace,
    TwinRails,
    SplitClamps
};

[[nodiscard]] inline GateStructureFamily gateStructureFamilyAt(
    std::uint64_t seed, std::uint32_t gateIndex) noexcept {
    const float roll = courseUnit(
        seed, static_cast<std::int64_t>(gateIndex), kGateStructureVisualChannel);
    const auto selected = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(roll * static_cast<float>(kGateStructureFamilyCount)),
        static_cast<std::uint32_t>(kGateStructureFamilyCount - 1U));
    return static_cast<GateStructureFamily>(selected);
}

[[nodiscard]] inline std::size_t gateStructureFamilyIndex(
    GateStructureFamily family) noexcept {
    switch (family) {
    case GateStructureFamily::RadialCage: return 0U;
    case GateStructureFamily::SegmentedCrown: return 1U;
    case GateStructureFamily::ChevronBrace: return 2U;
    case GateStructureFamily::TwinRails: return 3U;
    case GateStructureFamily::SplitClamps: return 4U;
    }
    return kGateStructureFamilyCount;
}

[[nodiscard]] inline const char* gateStructureFamilyName(
    GateStructureFamily family) noexcept {
    switch (family) {
    case GateStructureFamily::RadialCage: return "RADIAL CAGE";
    case GateStructureFamily::SegmentedCrown: return "SEGMENTED CROWN";
    case GateStructureFamily::ChevronBrace: return "CHEVRON BRACE";
    case GateStructureFamily::TwinRails: return "TWIN RAILS";
    case GateStructureFamily::SplitClamps: return "SPLIT CLAMPS";
    }
    return "UNKNOWN";
}

} // namespace tunrun
