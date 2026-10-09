#pragma once
#include <cmath>
#include <cstddef>
#include <vector>

namespace tunrun {
// raylib trigger axes commonly rest at -1 and move above zero when pressed.
[[nodiscard]] inline bool triggerPressed(float axisValue) noexcept {
    return std::isfinite(axisValue) && axisValue > 0.0F;
}
} // namespace tunrun

namespace tunrun {
enum class Screen { MainMenu, Hangar, Records, Controls, Modes, Settings, SettingsResetConfirm, Credits, Preview, Pause, RunConfirm, ExitConfirm, Crash, SeedLab, SeedEntry, SaveRecovery };

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
} // namespace tunrun
