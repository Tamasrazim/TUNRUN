#pragma once

#include "app/procedural_course.hpp"
#include "app/ship_catalog.hpp"

#include <algorithm>
#include <array>
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
    float pitch = 0.0F;
    float yaw = 0.0F;
    float roll = 0.0F;
    float pitchRate = 0.0F;
    float yawRate = 0.0F;
    float rollRate = 0.0F;
};

struct FlightInput {
    float steerX = 0.0F; // lateral translation: -1 left, +1 right
    float steerY = 0.0F; // lateral translation: -1 down, +1 up
    bool boost = false;
    bool precision = false;
    std::uint32_t shipId = kStarterShipId;
    bool dash = false;
    float rotateYaw = 0.0F;
    float rotatePitch = 0.0F;
    float roll = 0.0F;
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
    input.rotateYaw = std::isfinite(input.rotateYaw)
        ? std::clamp(input.rotateYaw, -1.0F, 1.0F) : 0.0F;
    input.rotatePitch = std::isfinite(input.rotatePitch)
        ? std::clamp(input.rotatePitch, -1.0F, 1.0F) : 0.0F;
    input.roll = std::isfinite(input.roll)
        ? std::clamp(input.roll, -1.0F, 1.0F) : 0.0F;

    state.yawRate = approach(state.yawRate, input.rotateYaw * 1.8F, 6.0F * dt);
    state.pitchRate = approach(state.pitchRate, input.rotatePitch * 1.45F, 5.0F * dt);
    state.rollRate = approach(state.rollRate, input.roll * 2.8F, 8.0F * dt);
    state.yaw = std::remainder(state.yaw + state.yawRate * dt,
                               2.0F * 3.14159265358979323846F);
    state.pitch = std::clamp(state.pitch + state.pitchRate * dt, -1.05F, 1.05F);
    state.roll = std::remainder(state.roll + state.rollRate * dt,
                               2.0F * 3.14159265358979323846F);

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
    // The ship's heading now contributes to its drift vector: rotating the
    // nose changes flight instead of being a cosmetic-only animation.
    const float headingDriftX = std::sin(state.yaw) * maximumSpeed * 0.28F;
    const float headingDriftY = -std::sin(state.pitch) * maximumSpeed * 0.22F;
    state.velocityX = approach(state.velocityX,
        input.steerX * maximumSpeed + headingDriftX, acceleration * dt);
    state.velocityY = approach(state.velocityY,
        input.steerY * maximumSpeed + headingDriftY, acceleration * dt);
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

 
// Bounded state-propagating route search. Each viable gate-crossing state
// branches into multiple in-aperture target policies for the next gate. The
// beam cap keeps validation deterministic and bounded; this is multi-trajectory
// coverage, not a formal proof of every physically reachable state.
struct StateGraphRouteValidation {
    bool valid = false;
    std::uint32_t gatesChecked = 0U;
    std::uint32_t transitionsChecked = 0U;
    std::uint32_t simulationSteps = 0U;
    std::uint32_t candidateStatesGenerated = 0U;
    std::uint32_t discardedStates = 0U;
    std::uint32_t beamPrunedStates = 0U;
    std::uint32_t peakStateCount = 0U;
    std::uint32_t gatesWithMultiplePassingStates = 0U;
    std::uint32_t maximumPassingStatesAtGate = 0U;
    std::uint32_t firstFailedGate = 0U;
    float minimumGateClearance = std::numeric_limits<float>::infinity();
    float maximumLateralOffset = 0.0F;
    double simulatedDistance = 0.0;
    const char* failure = "not validated";
};

inline StateGraphRouteValidation validateStateGraphRouteReachability(
    std::uint64_t seed, std::uint32_t gateCount = 12U,
    std::uint32_t shipId = kStarterShipId) noexcept {
    StateGraphRouteValidation result;
    if (gateCount == 0U || gateCount > 512U) {
        result.failure = "invalid gate count";
        return result;
    }
    if (shipId >= kShipCatalog.size()) {
        result.failure = "unknown ship profile";
        return result;
    }
    const auto obstacleValidation = validateObstacleSet(seed, gateCount);
    if (!obstacleValidation.valid) {
        result.failure = obstacleValidation.failure;
        return result;
    }

    constexpr std::size_t kMaximumStates = 32U;
    constexpr std::array<std::array<float, 2>, 9> kAimBiases{{
        {{ 0.00F,  0.00F}},
        {{-0.55F,  0.00F}},
        {{ 0.55F,  0.00F}},
        {{ 0.00F, -0.55F}},
        {{ 0.00F,  0.55F}},
        {{-0.35F, -0.35F}},
        {{-0.35F,  0.35F}},
        {{ 0.35F, -0.35F}},
        {{ 0.35F,  0.35F}}
    }};
    struct CandidateState {
        FlightState state{};
        float aimBiasX = 0.0F;
        float aimBiasY = 0.0F;
        bool active = true;
    };
    std::array<CandidateState, kMaximumStates> states{};
    std::array<FlightState, kMaximumStates> passingStates{};
    std::size_t stateCount = kAimBiases.size();
    std::size_t passingCount = 0U;
    const FlightState initialState{};
    for (std::size_t i = 0U; i < stateCount; ++i) {
        states[i].state = initialState;
        states[i].aimBiasX = kAimBiases[i][0];
        states[i].aimBiasY = kAimBiases[i][1];
    }
    result.candidateStatesGenerated = static_cast<std::uint32_t>(stateCount);
    result.peakStateCount = static_cast<std::uint32_t>(stateCount);

    const TunnelCrossSection conservativeTunnel{
        0.0F, 0.0F, kCourseMinRadius, 0.0F
    };
    ProceduralGate activeGate = gateAt(seed, 0U);
    const std::uint64_t stepBudget = std::min<std::uint64_t>(
        1000000ULL, 1024ULL + static_cast<std::uint64_t>(gateCount) * 900ULL);

    for (std::uint64_t step = 0U; step < stepBudget; ++step) {
        std::size_t activeCount = 0U;
        for (std::size_t i = 0U; i < stateCount; ++i) {
            if (states[i].active) ++activeCount;
        }

        if (activeCount == 0U) {
            if (passingCount == 0U) {
                result.firstFailedGate = activeGate.index;
                result.failure = "no candidate state clears the next gate";
                return result;
            }

            result.gatesChecked = activeGate.index + 1U;
            result.maximumPassingStatesAtGate = std::max(
                result.maximumPassingStatesAtGate,
                static_cast<std::uint32_t>(passingCount));
            if (passingCount > 1U) ++result.gatesWithMultiplePassingStates;
            if (result.gatesChecked >= gateCount) {
                result.valid = true;
                result.failure = "ok (bounded state graph)";
                return result;
            }

            ++result.transitionsChecked;
            activeGate = gateAt(seed, result.gatesChecked);
            std::array<CandidateState, kMaximumStates> nextStates{};
            const std::size_t totalBranches = passingCount * kAimBiases.size();
            const std::size_t nextCount = std::min(kMaximumStates, totalBranches);
            // Stratify the fixed beam across both parent trajectories and aim
            // policies. A policy-major loop would fill the beam with only the
            // first three/four policies whenever many parents survive.
            for (std::size_t slot = 0U; slot < nextCount; ++slot) {
                const std::size_t parent = (slot * passingCount) / nextCount;
                const auto& bias = kAimBiases[slot % kAimBiases.size()];
                nextStates[slot].state = passingStates[parent];
                nextStates[slot].aimBiasX = bias[0];
                nextStates[slot].aimBiasY = bias[1];
                nextStates[slot].active = true;
            }
            result.beamPrunedStates += static_cast<std::uint32_t>(
                totalBranches - nextCount);
            states = nextStates;
            stateCount = nextCount;
            passingCount = 0U;
            result.candidateStatesGenerated += static_cast<std::uint32_t>(totalBranches);
            result.peakStateCount = std::max(
                result.peakStateCount, static_cast<std::uint32_t>(stateCount));
            continue;
        }

        ++result.simulationSteps;
        for (std::size_t i = 0U; i < stateCount; ++i) {
            auto& candidate = states[i];
            if (!candidate.active) continue;

            const double previousDistance =
                static_cast<double>(candidate.state.distance);
            const float previousX = candidate.state.x;
            const float previousY = candidate.state.y;
            const auto& ship = shipDefinition(shipId);
            const float maximumLateralSpeed =
                (candidate.state.boostEnergy > 0.0F ? 6.0F : 4.0F) *
                ship.speedMultiplier;
            const float aimSpan = std::max(
                0.0F, activeGate.apertureRadius - kCraftCollisionRadius - 0.16F);
            const float aimX = activeGate.offsetX + candidate.aimBiasX * aimSpan;
            const float aimY = activeGate.offsetY + candidate.aimBiasY * aimSpan;
            const float steerX = std::clamp(
                ((aimX - candidate.state.x) * 2.8F -
                 candidate.state.velocityX * 1.25F) / maximumLateralSpeed,
                -1.0F, 1.0F);
            const float steerY = std::clamp(
                ((aimY - candidate.state.y) * 2.8F -
                 candidate.state.velocityY * 1.25F) / maximumLateralSpeed,
                -1.0F, 1.0F);
            updateFlight(candidate.state,
                FlightInput{steerX, steerY, true, false, shipId},
                kFlightFixedStep);

            const auto& state = candidate.state;
            if (!std::isfinite(state.x) || !std::isfinite(state.y) ||
                !std::isfinite(state.velocityX) || !std::isfinite(state.velocityY) ||
                !std::isfinite(state.distance) || !std::isfinite(state.boostEnergy)) {
                candidate.active = false;
                ++result.discardedStates;
                continue;
            }
            result.maximumLateralOffset = std::max(
                result.maximumLateralOffset, std::hypot(state.x, state.y));
            result.simulatedDistance = std::max(
                result.simulatedDistance, static_cast<double>(state.distance));
            if (collidesWithTunnelWall(state.x, state.y, conservativeTunnel)) {
                candidate.active = false;
                ++result.discardedStates;
                continue;
            }

            if (!crossesGatePlane(previousDistance,
                                  static_cast<double>(state.distance), activeGate)) {
                continue;
            }

            const double travel =
                static_cast<double>(state.distance) - previousDistance;
            const float fraction = travel > 1.0e-6
                ? static_cast<float>(std::clamp(
                    (activeGate.distance - previousDistance) / travel,
                    0.0, 1.0))
                : 0.0F;
            const float crossingX = previousX + (state.x - previousX) * fraction;
            const float crossingY = previousY + (state.y - previousY) * fraction;
            const float offsetX = crossingX - activeGate.offsetX;
            const float offsetY = crossingY - activeGate.offsetY;
            const float clearance = activeGate.apertureRadius -
                kCraftCollisionRadius - std::hypot(offsetX, offsetY);
            candidate.active = false;
            if (collidesWithGate(crossingX, crossingY, activeGate)) {
                ++result.discardedStates;
                continue;
            }

            result.minimumGateClearance = std::min(
                result.minimumGateClearance, clearance);
            if (passingCount < kMaximumStates) {
                passingStates[passingCount++] = candidate.state;
            }
        }
    }

    result.firstFailedGate = activeGate.index;
    result.failure = "bounded state graph step budget exhausted";
    return result;
}

} // namespace tunrun
