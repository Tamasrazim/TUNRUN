/*
 * What this does:
 * Works out screen positions and scaling so the interface stays readable at different window sizes.
 */

#pragma once

#include <algorithm>
#include <cmath>

namespace tunrun {

struct LogicalViewport {
    float x = 0.0F;
    float y = 0.0F;
    float width = 1280.0F;
    float height = 720.0F;
    float scale = 1.0F;
};

struct LogicalPoint {
    float x = 0.0F;
    float y = 0.0F;
};

// Fit the fixed 16:9 game canvas inside a physical window without stretching.
// Pointer coordinates use this exact viewport so letterbox bars never activate UI.
[[nodiscard]] inline LogicalViewport makeLogicalViewport(
    float screenWidth, float screenHeight,
    float canvasWidth = 1280.0F, float canvasHeight = 720.0F) noexcept {
    if (!std::isfinite(screenWidth) || !std::isfinite(screenHeight) ||
        !std::isfinite(canvasWidth) || !std::isfinite(canvasHeight) ||
        screenWidth <= 0.0F || screenHeight <= 0.0F ||
        canvasWidth <= 0.0F || canvasHeight <= 0.0F) {
        return {};
    }

    const float scale = std::min(screenWidth / canvasWidth,
                                 screenHeight / canvasHeight);
    if (!std::isfinite(scale) || scale <= 0.0F) return {};

    const float width = canvasWidth * scale;
    const float height = canvasHeight * scale;
    return LogicalViewport{
        (screenWidth - width) * 0.5F,
        (screenHeight - height) * 0.5F,
        width,
        height,
        scale
    };
}

[[nodiscard]] inline LogicalPoint toLogicalPointer(
    const LogicalViewport& viewport, float physicalX, float physicalY) noexcept {
    if (!std::isfinite(viewport.scale) || viewport.scale <= 0.0F ||
        !std::isfinite(physicalX) || !std::isfinite(physicalY)) {
        return {};
    }
    return LogicalPoint{
        (physicalX - viewport.x) / viewport.scale,
        (physicalY - viewport.y) / viewport.scale
    };
}

} // namespace tunrun
