#pragma once
#include <cmath>
#include <cstddef>
#include <vector>

namespace tunrun {
// raylib trigger axes commonly rest at -1 and move above zero when pressed.
[[nodiscard]] inline bool triggerPressed(float axisValue) noexcept {
    return std::isfinite(axisValue) && axisValue > 0.0F;
}

// Keyboard movement mapping is kept pure so it can be regression-tested
// without opening a raylib window or reading real keyboard hardware.
struct KeyboardFlightIntent {
    float lateral = 0.0F; // -1 left, +1 right
    float speed = 0.0F;   // -1 brake, +1 accelerate
    float yaw = 0.0F;     // -1 left, +1 right
    float pitch = 0.0F;   // -1 down, +1 up
    float roll = 0.0F;    // -1 left, +1 right
};

[[nodiscard]] inline KeyboardFlightIntent keyboardFlightIntentFromKeys(
    bool go, bool brake, bool left, bool rightB, bool rightD,
    bool yawLeft, bool yawRight, bool pitchUp, bool pitchDown,
    bool rollLeft, bool rollRight) noexcept {
    return KeyboardFlightIntent{
        static_cast<float>((rightB || rightD) - left),
        static_cast<float>(go - brake),
        static_cast<float>(yawRight - yawLeft),
        static_cast<float>(pitchUp - pitchDown),
        static_cast<float>(rollRight - rollLeft)
    };
}
} // namespace tunrun

namespace tunrun {
inline constexpr int kDefaultTargetFps = 144;
inline constexpr int kMaximumTargetFps = 240;

// Follow the monitor's reported refresh rate while bounding renderer load.
// A zero/implausibly low query falls back to the stable default target.
[[nodiscard]] constexpr int targetFpsForRefreshRate(int refreshRate) noexcept {
    if (refreshRate < 30) return kDefaultTargetFps;
    return refreshRate > kMaximumTargetFps ? kMaximumTargetFps : refreshRate;
}

enum class Screen { MainMenu, Hangar, HangarPurchaseConfirm, Records, Controls, Modes, Settings, SettingsResetConfirm, Credits, Preview, Pause, RunConfirm, ExitConfirm, Crash, SeedLab, SeedEntry, SaveRecovery };

// Converts a held state into one activation on the down edge.
class ButtonEdge {
public:
    [[nodiscard]] bool update(bool down) noexcept {
        const bool pressed = down && !wasDown_;
        wasDown_ = down;
        return pressed;
    }
    void reset() noexcept { wasDown_ = false; }
private:
    bool wasDown_ = false;
};

// Settings opened from Pause returns to Pause; root cannot be popped.
class ScreenStack {
public:
    ScreenStack() : screens_{Screen::MainMenu} {}
    [[nodiscard]] Screen current() const noexcept { return screens_.back(); }
    [[nodiscard]] std::size_t size() const noexcept { return screens_.size(); }
    [[nodiscard]] bool canGoBack() const noexcept { return screens_.size() > 1U; }
    [[nodiscard]] bool contains(Screen target) const noexcept {
        for (const Screen screen : screens_) {
            if (screen == target) return true;
        }
        return false;
    }
    void push(Screen value) { screens_.push_back(value); }
    void replace(Screen value) noexcept { screens_.back() = value; }
    bool pop() noexcept {
        if (!canGoBack()) return false;
        screens_.pop_back();
        return true;
    }
    void reset() {
        screens_.clear();
        screens_.push_back(Screen::MainMenu);
    }
private:
    std::vector<Screen> screens_;
};

// The shared elapsed clock also drives procedural mine motion. A Pause screen
// anywhere in the stack freezes it, including while child dialogs are open.
[[nodiscard]] inline bool shouldAdvanceRunClock(const ScreenStack& screens) noexcept {
    return !screens.contains(Screen::Pause) &&
           screens.current() != Screen::Settings &&
           screens.current() != Screen::Crash;
}

// Nonessential dash streaks are suppressed by Reduced Motion; physics and
// the textual dash-status indicator remain unchanged.
[[nodiscard]] inline bool shouldDrawDashStreaks(bool reduceMotion,
                                                float dashRemaining) noexcept {
    return !reduceMotion && std::isfinite(dashRemaining) && dashRemaining > 0.0F;
}
} // namespace tunrun
