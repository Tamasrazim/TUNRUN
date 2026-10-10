#pragma once

#include "app/tunnel_frame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

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

struct CameraSafeOffset {
    float right = 0.0F;
    float up = 0.0F;
    float radialOffset = 0.0F;
    bool clampedToTunnel = false;
};

// Keep the FPP camera inside the local sampled cross-section. A tight turn or
// radius transition can make the previous-frame camera section smaller than
// the ship's current section, even when the ship itself has not collided.
[[nodiscard]] inline CameraSafeOffset cameraSafeOffset(
    float right, float up, float tunnelRadius,
    float wallClearance = 0.55F) noexcept {
    bool clamped = false;
    if (!std::isfinite(right)) { right = 0.0F; clamped = true; }
    if (!std::isfinite(up)) { up = 0.0F; clamped = true; }
    if (!std::isfinite(tunnelRadius) || tunnelRadius <= 0.0F) {
        tunnelRadius = 5.0F;
        clamped = true;
    }
    if (!std::isfinite(wallClearance) || wallClearance < 0.0F) {
        wallClearance = 0.55F;
        clamped = true;
    }

    const float safeRadius = std::max(0.25F, tunnelRadius - wallClearance);
    float radial = std::hypot(right, up);
    if (!std::isfinite(radial)) {
        right = 0.0F;
        up = 0.0F;
        radial = 0.0F;
        clamped = true;
    } else if (radial > safeRadius) {
        const float scale = safeRadius / radial;
        right *= scale;
        up *= scale;
        radial = safeRadius;
        clamped = true;
    }
    return {right, up, radial, clamped};
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

// Smooth only the chase camera's lateral/vertical offset. The tunnel frame
// is resampled every frame, so smoothing world-space XYZ would lag the camera
// behind the moving course origin and can cut across curved walls.
struct CameraOrbitOffset {
    float right = 0.0F;
    float up = 0.0F;
    float behindDistance = 6.0F;
};

// Convert mouse-look yaw/pitch into an orbit around the spacecraft.
// Positive yaw moves the camera right; positive pitch lifts it above the ship.
// behindDistance can become negative to look around toward the front. The
// caller clamps the eye point against the active tunnel/throat before rendering.
[[nodiscard]] inline CameraOrbitOffset cameraOrbitOffset(
    float yaw, float pitch, float orbitRadius = 6.0F) noexcept {
    if (!std::isfinite(yaw)) yaw = 0.0F;
    if (!std::isfinite(pitch)) pitch = 0.0F;
    if (!std::isfinite(orbitRadius) || orbitRadius <= 0.0F) orbitRadius = 6.0F;
    yaw = std::remainder(yaw, 6.28318530717958647692F);
    pitch = std::clamp(pitch, -1.20F, 1.20F);
    const float horizontal = std::cos(pitch) * orbitRadius;
    return CameraOrbitOffset{
        std::sin(yaw) * horizontal,
        std::sin(pitch) * orbitRadius,
        std::cos(yaw) * horizontal
    };
}

struct CameraFollowState {
    float right = 0.0F;
    float up = 0.0F;
    bool initialized = false;
};

inline void smoothCameraFollow(CameraFollowState& state,
                               float targetRight, float targetUp,
                               float deltaTime,
                               float responsiveness = 14.0F) noexcept {
    if (!std::isfinite(targetRight)) targetRight = 0.0F;
    if (!std::isfinite(targetUp)) targetUp = 0.0F;
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0F) return;
    deltaTime = std::min(deltaTime, 0.1F);
    if (!std::isfinite(responsiveness) || responsiveness <= 0.0F) {
        responsiveness = 14.0F;
    }
    responsiveness = std::clamp(responsiveness, 0.1F, 40.0F);
    if (!state.initialized ||
        !std::isfinite(state.right) || !std::isfinite(state.up)) {
        state.right = targetRight;
        state.up = targetUp;
        state.initialized = true;
        return;
    }
    // Exponential decay has the same response at 30, 60, and 120+ Hz.
    const float blend = 1.0F - std::exp(-responsiveness * deltaTime);
    state.right += (targetRight - state.right) * blend;
    state.up += (targetUp - state.up) * blend;
}

struct CameraRayLimit {
    float safeFraction = 1.0F;
    float minimumWallClearance = 0.0F;
    bool clipped = false;
};

// A safe camera origin and a safe target can still be joined by a straight
// look ray that cuts through the inside wall on a tight curve. Sample that ray
// against the same procedural frames used by the renderer and shorten the
// centre view ray just before its first wall-clearance violation.
[[nodiscard]] inline CameraRayLimit limitCameraRayInsideTunnel(
    std::uint64_t seed, double referenceDistance,
    double cameraDistance, FrameVector3 cameraPosition,
    double targetDistance, FrameVector3 targetPosition,
    float wallClearance = 0.55F, unsigned int samples = 48U) noexcept {
    if (!std::isfinite(referenceDistance)) referenceDistance = 0.0;
    if (!std::isfinite(cameraDistance)) cameraDistance = referenceDistance;
    if (!std::isfinite(targetDistance)) targetDistance = cameraDistance;
    if (!std::isfinite(cameraPosition.x) || !std::isfinite(cameraPosition.y) ||
        !std::isfinite(cameraPosition.z) || !std::isfinite(targetPosition.x) ||
        !std::isfinite(targetPosition.y) || !std::isfinite(targetPosition.z)) {
        return CameraRayLimit{0.05F, 0.0F, true};
    }
    if (!std::isfinite(wallClearance) || wallClearance < 0.0F) wallClearance = 0.55F;
    samples = std::clamp(samples, 8U, 256U);

    const float dx = targetPosition.x - cameraPosition.x;
    const float dy = targetPosition.y - cameraPosition.y;
    const float dz = targetPosition.z - cameraPosition.z;
    float lastSafeFraction = 0.0F;
    float minimumClearance = std::numeric_limits<float>::infinity();

    for (unsigned int i = 0U; i <= samples; ++i) {
        const float fraction = static_cast<float>(i) / static_cast<float>(samples);
        const FrameVector3 point{
            cameraPosition.x + dx * fraction,
            cameraPosition.y + dy * fraction,
            cameraPosition.z + dz * fraction
        };
        const double frameDistance = cameraDistance +
            (targetDistance - cameraDistance) * static_cast<double>(fraction);
        const auto frame = sampleTunnelFrame(seed, referenceDistance, frameDistance);
        const FrameVector3 relative{
            point.x - frame.center.x,
            point.y - frame.center.y,
            point.z - frame.center.z
        };
        const float right = frameDot(relative, frame.right);
        const float up = frameDot(relative, frame.up);
        // Gate throats are smaller, bent interior surfaces nested inside the
        // main tube. Sampling only the outer tunnel radius would allow a
        // camera's straight look ray to cut across the sleeve wall even though
        // both endpoints were individually valid. Use the same throat sample
        // as live collisions and the visible mesh whenever one is active.
        const auto throat = gateThroatSectionAtDistance(seed, frameDistance);
        const float crossSectionCenterX = throat.active ? throat.centerX : 0.0F;
        const float crossSectionCenterY = throat.active ? throat.centerY : 0.0F;
        const float crossSectionRadius = throat.active ? throat.radius : frame.radius;
        const float radial = std::hypot(
            right - crossSectionCenterX, up - crossSectionCenterY);
        const float safeRadius = std::max(0.25F, crossSectionRadius - wallClearance);
        const float clearance = safeRadius - radial;
        minimumClearance = std::min(minimumClearance, clearance);
        if (!std::isfinite(radial) || radial > safeRadius) {
            // Back off more than one sample to leave margin for curvature
            // between sample points instead of stopping exactly at the wall.
            const float backoff = 1.5F / static_cast<float>(samples);
            const float safe = std::clamp(lastSafeFraction - backoff, 0.05F, 1.0F);
            return CameraRayLimit{safe, minimumClearance, true};
        }
        lastSafeFraction = fraction;
    }
    return CameraRayLimit{1.0F, minimumClearance, false};
}

} // namespace tunrun
