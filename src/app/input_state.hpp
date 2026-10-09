#pragma once
#include <cstddef>
#include <vector>

namespace tunrun {
enum class Screen { MainMenu, Hangar, Modes, Settings, Credits, Preview, Pause, ExitConfirm, Crash, SeedLab, SaveRecovery };

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
