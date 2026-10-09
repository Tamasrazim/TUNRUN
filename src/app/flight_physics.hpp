#pragma once

#include <algorithm>
#include <cmath>

namespace tunrun {

struct TunnelCrossSection {
    float centerX = 0.0F;
    float centerY = 0.0F;
    float radius = 5.5F;
};

struct FlightState {
    float x = 0.0F;
    float y = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float boostEnergy = 100.0F;
};

struct FlightInput {
    float steerX = 0.0F; // -1 left, +1 right
    float steerY = 0.0F; // -1 down, +1 up
    bool boost = false;
    bool precision = false;
};

inline constexpr float kFlightLimit = 6.5F;
inline constexpr float kCraftCollisionRadius = 0.42F;
inline constexpr float kFlightFixedStep = 1.0F / 120.0F;

inline TunnelCrossSection sampleTunnel(float time, float depth = 0.0F) noexcept {
    if (!std::isfinite(time)) time = 0.0F;
    if (!std::isfinite(depth) || depth < 0.0F) depth = 0.0F;
    const float centerX =
        std::sin(time * 0.55F + depth * 0.024F) * (0.25F + depth * 0.008F);
    const float centerY =
        std::sin(time * 0.35F + depth * 0.017F) * depth * 0.004F;
    const float radius =
        5.5F + 0.65F * std::sin(time * 0.7F + depth * 0.031F);
    return TunnelCrossSection{centerX, centerY, radius};
}

inline bool collidesWithTunnelWall(float x, float y,
                                   const TunnelCrossSection& section,
                                   float craftRadius = kCraftCollisionRadius) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(section.radius) || !std::isfinite(craftRadius)) return true;
    const float safeRadius = std::max(0.0F, section.radius - std::max(0.0F, craftRadius));
    const float dx = x - section.centerX;
    const float dy = y - section.centerY;
    return dx * dx + dy * dy >= safeRadius * safeRadius;
}

inline float approach(float current, float target, float maximumDelta) noexcept {
    if (current < target) return std::min(current + maximumDelta, target);
    return std::max(current - maximumDelta, target);
}

inline void updateFlight(FlightState& state, FlightInput input, float deltaTime) noexcept {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0F) return;
    const float dt = std::min(deltaTime, 0.05F);

    input.steerX = std::isfinite(input.steerX)
        ? std::clamp(input.steerX, -1.0F, 1.0F) : 0.0F;
    input.steerY = std::isfinite(input.steerY)
        ? std::clamp(input.steerY, -1.0F, 1.0F) : 0.0F;
    const float intentLength = std::sqrt(
        input.steerX * input.steerX + input.steerY * input.steerY);
    if (intentLength > 1.0F) {
        input.steerX /= intentLength;
        input.steerY /= intentLength;
    }

    const bool precision = input.precision;
    const bool boosting = input.boost && !precision && state.boostEnergy > 0.0F;
    const float maximumSpeed = precision ? 2.0F : (boosting ? 6.0F : 4.0F);
    const float acceleration = precision ? 16.0F : 10.0F;
    state.velocityX = approach(state.velocityX, input.steerX * maximumSpeed,
                               acceleration * dt);
    state.velocityY = approach(state.velocityY, input.steerY * maximumSpeed,
                               acceleration * dt);
    state.x += state.velocityX * dt;
    state.y += state.velocityY * dt;

    const float energyDelta = boosting ? -38.0F * dt : 18.0F * dt;
    state.boostEnergy = std::clamp(state.boostEnergy + energyDelta, 0.0F, 100.0F);

    if (state.x < -kFlightLimit) {
        state.x = -kFlightLimit;
        state.velocityX = std::max(0.0F, state.velocityX);
    } else if (state.x > kFlightLimit) {
        state.x = kFlightLimit;
        state.velocityX = std::min(0.0F, state.velocityX);
    }
    if (state.y < -kFlightLimit) {
        state.y = -kFlightLimit;
        state.velocityY = std::max(0.0F, state.velocityY);
    } else if (state.y > kFlightLimit) {
        state.y = kFlightLimit;
        state.velocityY = std::min(0.0F, state.velocityY);
    }
}

inline void advanceFlight(FlightState& state, FlightInput input,
                          float frameDelta, float& accumulator) noexcept {
    if (!std::isfinite(frameDelta) || frameDelta <= 0.0F) return;
    accumulator += std::min(frameDelta, 0.05F);
    while (accumulator + 1.0e-7F >= kFlightFixedStep) {
        updateFlight(state, input, kFlightFixedStep);
        accumulator -= kFlightFixedStep;
        if (accumulator < 0.0F) accumulator = 0.0F;
    }
}

} // namespace tunrun
