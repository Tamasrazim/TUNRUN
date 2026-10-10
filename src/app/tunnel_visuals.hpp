#pragma once

#include "app/procedural_course.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {

inline constexpr double kTunnelVisualSectionLength = 24.0;
inline constexpr std::uint64_t kTunnelVisualFamilyChannel = 920U;
inline constexpr std::uint64_t kTunnelVisualIntensityChannel = 921U;

enum class TunnelVisualFamily : std::uint8_t {
    RibbedMetal, PlasmaRails, FracturedPanels, SpiralConduits, Lattice
};

struct TunnelVisualPalette {
    std::uint8_t red = 140U;
    std::uint8_t green = 173U;
    std::uint8_t blue = 206U;
};

struct TunnelVisualProfile {
    TunnelVisualFamily family = TunnelVisualFamily::RibbedMetal;
    TunnelVisualPalette palette{};
    std::uint8_t railMask = 0x09U;
    float intensity = 0.8F;
};

[[nodiscard]] inline std::int64_t tunnelVisualSectionIndex(
    double worldDistance) noexcept {
    if (!std::isfinite(worldDistance)) return 0;
    const double section = std::floor(worldDistance / kTunnelVisualSectionLength);
    if (section <= static_cast<double>(std::numeric_limits<std::int64_t>::min()))
        return std::numeric_limits<std::int64_t>::min();
    if (section >= static_cast<double>(std::numeric_limits<std::int64_t>::max()))
        return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>(section);
}

[[nodiscard]] inline TunnelVisualFamily tunnelVisualFamilyAt(
    std::uint64_t seed, std::int64_t sectionIndex) noexcept {
    const float roll = courseUnit(seed, sectionIndex, kTunnelVisualFamilyChannel);
    if (roll < 0.20F) return TunnelVisualFamily::RibbedMetal;
    if (roll < 0.40F) return TunnelVisualFamily::PlasmaRails;
    if (roll < 0.60F) return TunnelVisualFamily::FracturedPanels;
    if (roll < 0.80F) return TunnelVisualFamily::SpiralConduits;
    return TunnelVisualFamily::Lattice;
}

[[nodiscard]] inline bool validTunnelVisualFamily(
    TunnelVisualFamily family) noexcept {
    switch (family) {
    case TunnelVisualFamily::RibbedMetal:
    case TunnelVisualFamily::PlasmaRails:
    case TunnelVisualFamily::FracturedPanels:
    case TunnelVisualFamily::SpiralConduits:
    case TunnelVisualFamily::Lattice:
        return true;
    }
    return false;
}

[[nodiscard]] inline const char* tunnelVisualFamilyName(
    TunnelVisualFamily family) noexcept {
    switch (family) {
    case TunnelVisualFamily::RibbedMetal: return "RIBBED METAL";
    case TunnelVisualFamily::PlasmaRails: return "PLASMA RAILS";
    case TunnelVisualFamily::FracturedPanels: return "FRACTURED PANELS";
    case TunnelVisualFamily::SpiralConduits: return "SPIRAL CONDUITS";
    case TunnelVisualFamily::Lattice: return "LATTICE";
    }
    return "UNKNOWN TUNNEL STYLE";
}

[[nodiscard]] inline TunnelVisualProfile tunnelVisualProfileAt(
    std::uint64_t seed, std::int64_t sectionIndex) noexcept {
    TunnelVisualProfile profile;
    profile.family = tunnelVisualFamilyAt(seed, sectionIndex);
    profile.intensity = 0.58F +
        courseUnit(seed, sectionIndex, kTunnelVisualIntensityChannel) * 0.34F;
    switch (profile.family) {
    case TunnelVisualFamily::RibbedMetal:
        profile.palette = {145U, 177U, 211U};
        profile.railMask = 0x09U;
        break;
    case TunnelVisualFamily::PlasmaRails:
        profile.palette = {78U, 199U, 255U};
        profile.railMask = 0x2DU;
        break;
    case TunnelVisualFamily::FracturedPanels:
        profile.palette = {173U, 126U, 242U};
        profile.railMask = 0x15U;
        break;
    case TunnelVisualFamily::SpiralConduits:
        profile.palette = {76U, 223U, 190U};
        profile.railMask = 0x3FU;
        break;
    case TunnelVisualFamily::Lattice:
        profile.palette = {236U, 166U, 107U};
        profile.railMask = 0x1BU;
        break;
    }
    return profile;
}

[[nodiscard]] inline bool tunnelVisualRailEnabled(
    const TunnelVisualProfile& profile, int railIndex) noexcept {
    if (railIndex < 0 || railIndex >= 6) return false;
    return (profile.railMask & static_cast<std::uint8_t>(1U << railIndex)) != 0U;
}

} // namespace tunrun
