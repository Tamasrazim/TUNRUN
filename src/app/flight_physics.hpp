#pragma once

#include "app/procedural_course.hpp"
#include "app/ship_catalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace tunrun {

struct FlightState {
    float x = 0.0F;
    float y = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float boostEnergy = 100.0F;
    float distance = 0.0F;
    float dashRemaining = 0.0F;
    float dashCooldownRemaining = 0.0F;
    bool dashButtonWasDown = false;
};

struct FlightInput {
    float steerX = 0.0F; // -1 left, +1 right
    float steerY = 0.0F; // -1 down, +1 up
    bool boost = false;
    bool precision = false;
    std::uint32_t shipId = kStarterShipId;
    bool dash = false;
};

inline constexpr float kFlightLimit = 6.5F;
inline constexpr float kFlightFixedStep = 1.0F / 120.0F;
inline constexpr float kDashEnergyCost = 28.0F;
inline constexpr float kDashDuration = 0.24F;
inline constexpr float kDashCooldown = 1.20F;

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

    const auto& ship = shipDefinition(input.shipId);
    const bool precision = input.precision;
    const bool dashPressedEdge = input.dash && !state.dashButtonWasDown;
    state.dashButtonWasDown = input.dash;
    state.dashCooldownRemaining = std::max(0.0F, state.dashCooldownRemaining - dt);
    state.dashRemaining = std::max(0.0F, state.dashRemaining - dt);
    if (dashPressedEdge && !precision &&
        state.dashCooldownRemaining <= 0.0F &&
        state.boostEnergy >= kDashEnergyCost) {
        state.boostEnergy -= kDashEnergyCost;
        state.dashRemaining = kDashDuration;
        state.dashCooldownRemaining = kDashCooldown;
    }
    const bool boosting = input.boost && !precision && state.boostEnergy > 0.0F;
    const float maximumSpeed =
        (precision ? 2.0F : (boosting ? 6.0F : 4.0F)) * ship.speedMultiplier;
    const float acceleration =
        (precision ? 16.0F : 10.0F) * ship.accelerationMultiplier;
    state.velocityX = approach(state.velocityX, input.steerX * maximumSpeed,
                               acceleration * dt);
    state.velocityY = approach(state.velocityY, input.steerY * maximumSpeed,
                               acceleration * dt);
    state.x += state.velocityX * dt;
    state.y += state.velocityY * dt;
    const float forwardSpeed = (precision ? 8.0F
        : state.dashRemaining > 0.0F ? (boosting ? 24.0F : 19.0F)
        : boosting ? 16.0F : 11.0F) * ship.speedMultiplier;
    state.distance += forwardSpeed * dt;

    const float energyDelta = boosting
        ? -38.0F * dt * ship.boostDrainMultiplier
        : 18.0F * dt * ship.energyRegenerationMultiplier;
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


// A deterministic route probe propagates one real kinematic state through the
// generated course at the same fixed step used by gameplay. It is a concrete
// feasible-trajectory witness, not an exhaustive reachable-state proof.
struct SimulatedRouteValidation {
    bool valid = false;
    std::uint32_t gatesChecked = 0U;
    std::uint32_t transitionsChecked = 0U;
    std::uint32_t simulationSteps = 0U;
    std::uint32_t firstFailedGate = 0U;
    float minimumGateClearance = std::numeric_limits<float>::infinity();
    float maximumLateralOffset = 0.0F;
    double simulatedDistance = 0.0;
    const char* failure = "not validated";
};

inline SimulatedRouteValidation validateSimulatedRouteReachability(
    std::uint64_t seed, std::uint32_t gateCount = 128U,
    std::uint32_t shipId = kStarterShipId) noexcept {
    SimulatedRouteValidation result;
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

    const auto& ship = shipDefinition(shipId);
    FlightState state;
    ProceduralGate activeGate = gateAt(seed, 0U);
    // Validate against the narrowest permitted tunnel, not a convenient
    // per-sample radius, so a passing probe cannot depend on a wide segment.
    const TunnelCrossSection conservativeTunnel{
        0.0F, 0.0F, kCourseMinRadius, 0.0F
    };
    const std::uint64_t stepBudget = std::min<std::uint64_t>(
        8000000ULL, 1024ULL + static_cast<std::uint64_t>(gateCount) * 800ULL);

    for (std::uint64_t step = 0; step < stepBudget; ++step) {
        if (result.gatesChecked >= gateCount) {
            result.valid = true;
            result.failure = "ok";
            return result;
        }

        const double previousDistance = static_cast<double>(state.distance);
        const float previousX = state.x;
        const float previousY = state.y;

        // A bounded feedback pilot aims for the center of the upcoming
        // aperture while damping lateral velocity. The simulation below uses
        // updateFlight(), not a separate idealised motion equation.
        const float maximumLateralSpeed =
            (state.boostEnergy > 0.0F ? 6.0F : 4.0F) * ship.speedMultiplier;
        const float steerX = std::clamp(
            ((activeGate.offsetX - state.x) * 2.8F -
             state.velocityX * 1.25F) / maximumLateralSpeed, -1.0F, 1.0F);
        const float steerY = std::clamp(
            ((activeGate.offsetY - state.y) * 2.8F -
             state.velocityY * 1.25F) / maximumLateralSpeed, -1.0F, 1.0F);
        updateFlight(state, FlightInput{steerX, steerY, true, false, shipId},
                     kFlightFixedStep);
        ++result.simulationSteps;

        if (!std::isfinite(state.x) || !std::isfinite(state.y) ||
            !std::isfinite(state.velocityX) || !std::isfinite(state.velocityY) ||
            !std::isfinite(state.distance) || !std::isfinite(state.boostEnergy)) {
            result.firstFailedGate = activeGate.index;
            result.failure = "non-finite simulated flight state";
            return result;
        }
        result.maximumLateralOffset = std::max(
            result.maximumLateralOffset, std::sqrt(state.x * state.x + state.y * state.y));
        result.simulatedDistance = static_cast<double>(state.distance);
        if (collidesWithTunnelWall(state.x, state.y, conservativeTunnel)) {
            result.firstFailedGate = activeGate.index;
            result.failure = "simulated route intersects minimum tunnel clearance";
            return result;
        }

        if (!crossesGatePlane(previousDistance,
                              static_cast<double>(state.distance), activeGate)) {
            continue;
        }

        const double travel = static_cast<double>(state.distance) - previousDistance;
        const float fraction = travel > 1.0e-6
            ? static_cast<float>(std::clamp(
                (activeGate.distance - previousDistance) / travel, 0.0, 1.0))
            : 0.0F;
        const float crossingX = previousX + (state.x - previousX) * fraction;
        const float crossingY = previousY + (state.y - previousY) * fraction;
        const float offsetX = crossingX - activeGate.offsetX;
        const float offsetY = crossingY - activeGate.offsetY;
        const float clearance = activeGate.apertureRadius - kCraftCollisionRadius -
            std::sqrt(offsetX * offsetX + offsetY * offsetY);
        result.minimumGateClearance = std::min(result.minimumGateClearance, clearance);

        if (collidesWithGate(crossingX, crossingY, activeGate)) {
            result.firstFailedGate = activeGate.index;
            result.failure = "simulated route misses a gate aperture";
            return result;
        }

        ++result.gatesChecked;
        if (result.gatesChecked >= gateCount) {
            result.valid = true;
            result.failure = "ok";
            return result;
        }
        ++result.transitionsChecked;
        activeGate = gateAt(seed, result.gatesChecked);
    }

    result.firstFailedGate = result.gatesChecked;
    result.failure = "route simulation step budget exhausted";
    return result;
}

} // namespace tunrun
