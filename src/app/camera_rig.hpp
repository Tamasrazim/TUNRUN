#pragma once

#include <algorithm>
#include <cmath>

namespace tunrun {

struct CameraLookOffset {
    float right = 0.0F;
    float up = 0.0F;
};

// Convert yaw/pitch into a target offset on the tunnel cross-section. The
// target is clamped away from the wall so looking while turning cannot point
// at a target outside the tube.
[[nodiscard]] inline CameraLookOffset cameraLookOffset(
    float yaw, float pitch, float tunnelRadius,
    float lookDistance = 8.0F) noexcept {
    if (!std::isfinite(yaw)) yaw = 0.0F;
    if (!std::isfinite(pitch)) pitch = 0.0F;
    if (!std::isfinite(tunnelRadius) || tunnelRadius <= 0.0F) tunnelRadius = 5.0F;
    if (!std::isfinite(lookDistance) || lookDistance <= 0.0F) lookDistance = 8.0F;
    yaw = std::remainder(yaw, 6.28318530717958647692F);
    pitch = std::clamp(pitch, -1.05F, 1.05F);
    float right = std::sin(yaw) * std::cos(pitch) * lookDistance;
    float up = std::sin(pitch) * lookDistance;
    const float radial = std::hypot(right, up);
    const float maximumOffset = std::clamp(tunnelRadius - 1.1F, 0.25F, 3.2F);
    if (!std::isfinite(radial)) return {};
    if (radial > maximumOffset && radial > 1.0e-5F) {
        const float scale = maximumOffset / radial;
        right *= scale;
        up *= scale;
    }
    return {right, up};
}

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
