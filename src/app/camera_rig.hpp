#pragma once

#include <algorithm>
#include <cmath>

namespace tunrun {

struct ThirdPersonCameraPose {
    float x = 0.0F;
    float y = 0.25F;
    float z = 6.0F;
    float radialOffset = 0.25F;
    bool clampedToTunnel = false;
};

// Place the chase camera behind the ship relative to the rear tunnel section.
// The desired offset is measured from that section's centerline, then clamped
// to preserve wall clearance even where the procedural centerline is bending.
[[nodiscard]] inline ThirdPersonCameraPose thirdPersonCameraPose(
    float shipX, float shipY,
    float forwardX, float forwardY, float forwardZ,
    float rearCenterX, float rearCenterY, float rearRadius,
    float followDistance = 6.0F, float verticalLift = 0.25F,
    float wallClearance = 1.0F) noexcept {
    if (!std::isfinite(shipX)) shipX = 0.0F;
    if (!std::isfinite(shipY)) shipY = 0.0F;
    if (!std::isfinite(forwardX)) forwardX = 0.0F;
    if (!std::isfinite(forwardY)) forwardY = 0.0F;
    if (!std::isfinite(forwardZ)) forwardZ = -1.0F;
    if (!std::isfinite(rearCenterX)) rearCenterX = 0.0F;
    if (!std::isfinite(rearCenterY)) rearCenterY = 0.0F;
    if (!std::isfinite(rearRadius) || rearRadius <= 0.0F) rearRadius = 5.0F;
    if (!std::isfinite(followDistance) || followDistance <= 0.0F) followDistance = 6.0F;
    if (!std::isfinite(verticalLift)) verticalLift = 0.25F;
    if (!std::isfinite(wallClearance) || wallClearance < 0.0F) wallClearance = 1.0F;

    const float safeRadius = std::max(0.5F, rearRadius - wallClearance);
    float offsetX = shipX - forwardX * followDistance - rearCenterX;
    float offsetY = shipY - forwardY * followDistance + verticalLift - rearCenterY;
    float offsetLength = std::hypot(offsetX, offsetY);
    bool clamped = false;
    if (!std::isfinite(offsetLength)) {
        offsetX = 0.0F;
        offsetY = 0.0F;
        offsetLength = 0.0F;
        clamped = true;
    } else if (offsetLength > safeRadius) {
        const float scale = safeRadius / offsetLength;
        offsetX *= scale;
        offsetY *= scale;
        offsetLength = safeRadius;
        clamped = true;
    }

    return ThirdPersonCameraPose{
        rearCenterX + offsetX,
        rearCenterY + offsetY,
        -forwardZ * followDistance,
        offsetLength,
        clamped
    };
}

} // namespace tunrun
