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

// Sensitivity spans a large ratio, so the visible slider uses a logarithmic
// scale; this gives useful resolution at the low end without hiding high values.
[[nodiscard]] inline float mouseSensitivitySliderPosition(float sensitivity) noexcept {
    if (!std::isfinite(sensitivity)) sensitivity = kMouseSensitivityDefault;
    sensitivity = std::clamp(sensitivity, kMouseSensitivityMin, kMouseSensitivityMax);
    const double logMin = std::log(static_cast<double>(kMouseSensitivityMin));
    const double logMax = std::log(static_cast<double>(kMouseSensitivityMax));
    const double logValue = std::log(static_cast<double>(sensitivity));
    return static_cast<float>(std::clamp((logValue - logMin) / (logMax - logMin), 0.0, 1.0));
}

// Maps a normalized pointer position to sensitivity on the same logarithmic
// scale, snapping to the keyboard/controller increment grid centred at default.
[[nodiscard]] inline float mouseSensitivityFromSlider(float normalizedPosition) noexcept {
    if (!std::isfinite(normalizedPosition)) return kMouseSensitivityDefault;
    const float position = std::clamp(normalizedPosition, 0.0F, 1.0F);
    if (position <= 0.0F) return kMouseSensitivityMin;
    if (position >= 1.0F) return kMouseSensitivityMax;
    const double logMin = std::log(static_cast<double>(kMouseSensitivityMin));
    const double logMax = std::log(static_cast<double>(kMouseSensitivityMax));
    const float raw = static_cast<float>(std::exp(
        logMin + static_cast<double>(position) * (logMax - logMin)));
    const float steps = std::round((raw - kMouseSensitivityDefault) / kMouseSensitivityStep);
    return std::clamp(kMouseSensitivityDefault + steps * kMouseSensitivityStep,
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

[[nodiscard]] inline float mouseTargetSteering(
    float target, float position, float velocity,
    float gain = 0.78F, float damping = 0.30F) noexcept {
    if (!std::isfinite(target)) target = 0.0F;
    if (!std::isfinite(position)) position = 0.0F;
    if (!std::isfinite(velocity)) velocity = 0.0F;
    if (!std::isfinite(gain) || gain < 0.0F) gain = 0.78F;
    if (!std::isfinite(damping) || damping < 0.0F) damping = 0.30F;
    return std::clamp((target - position) * gain - velocity * damping,
                      -1.0F, 1.0F);
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
    // Refreshes client clipping after resize/move or display-mode changes.
    void refreshClip() noexcept;
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
    bool clipBoundsValid_ = false;
    std::int32_t clipLeft_ = 0;
    std::int32_t clipTop_ = 0;
    std::int32_t clipRight_ = 0;
    std::int32_t clipBottom_ = 0;
};
