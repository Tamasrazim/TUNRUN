#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

inline constexpr float kMouseSensitivityDefault = 0.004F;
inline constexpr float kMouseSensitivityMin = 0.0001F;
inline constexpr float kMouseSensitivityMax = 0.05F;
inline constexpr float kMouseSensitivityStep = 0.0005F;

[[nodiscard]] inline float adjustMouseSensitivity(float current,
                                                  int direction) noexcept {
    if (!std::isfinite(current)) current = kMouseSensitivityDefault;
    current = std::clamp(current, kMouseSensitivityMin, kMouseSensitivityMax);
    if (direction < 0) current -= kMouseSensitivityStep;
    else if (direction > 0) current += kMouseSensitivityStep;
    return std::clamp(current, kMouseSensitivityMin, kMouseSensitivityMax);
}

// Maps a pointer's normalized horizontal position onto the supported
// sensitivity range, quantized to the same step used by keyboard/controller input.
[[nodiscard]] inline float mouseSensitivityFromSlider(float normalizedPosition) noexcept {
    if (!std::isfinite(normalizedPosition)) return kMouseSensitivityDefault;
    const float position = std::clamp(normalizedPosition, 0.0F, 1.0F);
    const float raw = kMouseSensitivityMin +
        position * (kMouseSensitivityMax - kMouseSensitivityMin);
    const float steps = std::round((raw - kMouseSensitivityMin) / kMouseSensitivityStep);
    return std::clamp(kMouseSensitivityMin + steps * kMouseSensitivityStep,
                      kMouseSensitivityMin, kMouseSensitivityMax);
}

struct RelativeMouseDelta {
    float x = 0.0F;
    float y = 0.0F;
};

inline void applyRelativeMouseSteering(float& x, float& y,
                                       RelativeMouseDelta delta,
                                       float sensitivity,
                                       float limit = 3.1F) noexcept {
    if (!std::isfinite(sensitivity) || sensitivity < 0.0F) sensitivity = 0.0F;
    if (!std::isfinite(limit) || limit <= 0.0F) limit = 3.1F;
    if (!std::isfinite(delta.x)) delta.x = 0.0F;
    if (!std::isfinite(delta.y)) delta.y = 0.0F;
    x = std::clamp(x + delta.x * sensitivity, -limit, limit);
    y = std::clamp(y - delta.y * sensitivity, -limit, limit);
}

class RawMouse {
public:
    RawMouse() = default;
    RawMouse(const RawMouse&) = delete;
    RawMouse& operator=(const RawMouse&) = delete;
    ~RawMouse();

    bool install(void* nativeWindow) noexcept;
    [[nodiscard]] bool installed() const noexcept { return installed_; }
    void setActive(bool wanted) noexcept;
    [[nodiscard]] RelativeMouseDelta consume() noexcept;
    void clear() noexcept;
    void uninstall() noexcept;

    // Called by the native window-procedure bridge in raw_mouse.cpp.
    void collectNativeInput(std::intptr_t inputHandle) noexcept;
    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    std::atomic<long long> deltaX_{0};
    std::atomic<long long> deltaY_{0};
    bool installed_ = false;
    bool active_ = false;
};
