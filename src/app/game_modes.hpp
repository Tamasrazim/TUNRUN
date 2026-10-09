#pragma once

#include <cstdint>

namespace tunrun {

enum class GameModeChoice : std::uint8_t {
    Campaign = 0U,
    Endless = 1U,
    CustomSeedRun = 2U,
    PracticePreview = 3U,
    Back = 4U
};

[[nodiscard]] constexpr bool isModeImplemented(GameModeChoice choice) noexcept {
    return choice == GameModeChoice::CustomSeedRun ||
           choice == GameModeChoice::PracticePreview;
}

[[nodiscard]] constexpr const char* modeUnavailableMessage(
    GameModeChoice choice) noexcept {
    switch (choice) {
    case GameModeChoice::Campaign:
        return "Campaign mode is not implemented in this build; checkpoints and progression are still in development.";
    case GameModeChoice::Endless:
        return "A separate Endless mode is not implemented yet; use Play / Procedural Run for the current survival loop.";
    case GameModeChoice::CustomSeedRun:
    case GameModeChoice::PracticePreview:
    case GameModeChoice::Back:
        return "";
    }
    return "Unknown mode.";
}

} // namespace tunrun
