#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>

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
