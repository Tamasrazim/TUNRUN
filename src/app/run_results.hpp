#pragma once

#include <cstdint>

namespace tunrun {

enum class RunResultNotice : std::uint8_t {
    None,
    NewScore,
    NewCombo,
    NewScoreAndCombo,
    SaveFailure
};

// Persistence failure must take priority over a celebratory record notice:
// values held in memory are not guaranteed to survive the next launch.
[[nodiscard]] constexpr RunResultNotice classifyRunResultNotice(
    bool saveWarning, bool newBestScore, bool newBestCombo) noexcept {
    if (saveWarning) return RunResultNotice::SaveFailure;
    if (newBestScore && newBestCombo) return RunResultNotice::NewScoreAndCombo;
    if (newBestScore) return RunResultNotice::NewScore;
    if (newBestCombo) return RunResultNotice::NewCombo;
    return RunResultNotice::None;
}

[[nodiscard]] constexpr const char* runResultNoticeText(
    RunResultNotice notice) noexcept {
    switch (notice) {
    case RunResultNotice::None:
        return nullptr;
    case RunResultNotice::NewScore:
        return "NEW PERSONAL SCORE RECORD";
    case RunResultNotice::NewCombo:
        return "NEW PERSONAL COMBO RECORD";
    case RunResultNotice::NewScoreAndCombo:
        return "NEW SCORE + COMBO RECORD";
    case RunResultNotice::SaveFailure:
        return "SAVE FAILED: RUN RESULTS MAY NOT BE SAVED";
    }
    return "RUN RESULT STATUS UNAVAILABLE";
}

} // namespace tunrun
