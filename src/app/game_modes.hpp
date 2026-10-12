/*
 * What this does:
 * Keeps the different run modes and their rules separate, so the game knows what kind of session is active.
 */

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

// Practice keeps the same seed geometry and collision rules, but slows forward
// travel to 65% so new players have more time to read generated passages.
[[nodiscard]] constexpr float gameModeForwardSpeedScale(
    GameModeChoice choice) noexcept {
    return choice == GameModeChoice::PracticePreview ? 0.65F : 1.0F;
}

[[nodiscard]] constexpr bool isModeImplemented(GameModeChoice choice) noexcept {
    return choice == GameModeChoice::Endless ||
           choice == GameModeChoice::CustomSeedRun ||
           choice == GameModeChoice::PracticePreview;
}

[[nodiscard]] constexpr const char* gameModeName(GameModeChoice choice) noexcept {
    switch (choice) {
    case GameModeChoice::Campaign: return "CAMPAIGN";
    case GameModeChoice::Endless: return "ENDLESS";
    case GameModeChoice::CustomSeedRun: return "CUSTOM SEED";
    case GameModeChoice::PracticePreview: return "PRACTICE";
    case GameModeChoice::Back: return "MENU";
    }
    return "UNKNOWN";
}

[[nodiscard]] constexpr const char* modeUnavailableMessage(
    GameModeChoice choice) noexcept {
    switch (choice) {
    case GameModeChoice::Campaign:
        return "Campaign mode is not implemented in this build; checkpoints and progression are still in development.";
    case GameModeChoice::Endless:
        return "";
    case GameModeChoice::CustomSeedRun:
    case GameModeChoice::PracticePreview:
    case GameModeChoice::Back:
        return "";
    }
    return "Unknown mode.";
}

} // namespace tunrun
