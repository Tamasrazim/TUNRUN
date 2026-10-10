#include "app/input_state.hpp"
#include "app/camera_rig.hpp"
#include "app/tunnel_frame.hpp"
#include "app/flight_physics.hpp"
#include "app/rewards.hpp"
#include "app/hazards.hpp"
#include "app/scoring.hpp"
#include "app/seed_text.hpp"
#include "app/game_modes.hpp"
#include "app/run_results.hpp"
#include "app/economy.hpp"
#include "app/raw_mouse.hpp"
#include "app/save_profile.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
bool writeTestFile(const std::filesystem::path& path, std::string_view content) {
#if defined(_WIN32)
    // The production store hides these files; clear the cosmetic attribute so
    // the test can corrupt them deterministically with the Win32 file API.
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0U;
    const bool ok = WriteFile(file, content.data(),
        static_cast<DWORD>(content.size()), &written, nullptr) != 0 &&
        static_cast<std::size_t>(written) == content.size();
    CloseHandle(file);
    return ok;
#else
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(file);
#endif
}
} // namespace

int main() {
    using tunrun::ButtonEdge;
    using tunrun::Screen;
    using tunrun::ScreenStack;

    const tunrun::Profile defaultProfile{};
    assert(defaultProfile.fullscreen);
    assert(defaultProfile.mouseSteering);
    assert(defaultProfile.mouseSensitivity == kMouseSensitivityDefault);

    constexpr std::uint64_t frameSeed = 0xA91B72C3D4E5F607ULL;
    const auto curvedFrame = tunrun::sampleTunnelFrame(frameSeed, 32.0, 58.0);
    assert(std::abs(tunrun::frameLength(curvedFrame.tangent)-1.0F)<0.0001F);
    assert(std::abs(tunrun::frameLength(curvedFrame.right)-1.0F)<0.0001F);
    assert(std::abs(tunrun::frameLength(curvedFrame.up)-1.0F)<0.0001F);
    assert(std::abs(tunrun::frameDot(curvedFrame.tangent,curvedFrame.right))<0.0001F);
    assert(std::abs(tunrun::frameDot(curvedFrame.tangent,curvedFrame.up))<0.0001F);
    assert(std::abs(tunrun::frameDot(curvedFrame.right,curvedFrame.up))<0.0001F);
    const auto rim=tunrun::tunnelFramePoint(curvedFrame,curvedFrame.radius,0.0F);
    const tunrun::FrameVector3 rimOffset{rim.x-curvedFrame.center.x,
        rim.y-curvedFrame.center.y,rim.z-curvedFrame.center.z};
    assert(std::abs(tunrun::frameLength(rimOffset)-curvedFrame.radius)<0.001F);
    const auto badFrame=tunrun::sampleTunnelFrame(frameSeed,
        std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity());
    assert(std::isfinite(badFrame.center.x)&&std::isfinite(badFrame.center.y)&&
           std::isfinite(badFrame.center.z));

    const auto straightLook = tunrun::cameraLookOffset(0.0F, 0.0F, 5.75F);
    assert(std::abs(straightLook.right) < 0.0001F);
    assert(std::abs(straightLook.up) < 0.0001F);
    const auto sideLook = tunrun::cameraLookOffset(1.57079632679F, 0.0F, 5.75F);
    assert(sideLook.right > 3.0F && std::abs(sideLook.up) < 0.0001F);
    assert(std::hypot(sideLook.right, sideLook.up) <= 3.2001F);
    const auto upperLook = tunrun::cameraLookOffset(0.0F, 0.8F, 5.75F);
    assert(upperLook.up > 0.0F && std::abs(upperLook.right) < 0.0001F);
    assert(std::hypot(upperLook.right, upperLook.up) <= 3.2001F);
    const auto invalidLook = tunrun::cameraLookOffset(
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN());
    assert(std::isfinite(invalidLook.right) && std::isfinite(invalidLook.up));

    const auto safeChasePose = tunrun::thirdPersonCameraPose(
        0.5F, -0.3F, 0.0F, 0.0F, -1.0F, 1.0F, -0.5F, 5.0F);
    assert(!safeChasePose.clampedToTunnel);
    assert(std::abs(safeChasePose.x - 0.5F) < 0.0001F);
    assert(std::abs(safeChasePose.y + 0.05F) < 0.0001F);
    assert(std::abs(safeChasePose.z - 6.0F) < 0.0001F);

    const auto clampedChasePose = tunrun::thirdPersonCameraPose(
        2.0F, 1.0F, 1.5F, 1.3F, -0.2F, -1.0F, 0.75F, 4.0F);
    assert(clampedChasePose.clampedToTunnel);
    assert(clampedChasePose.radialOffset <= 3.0001F);
    assert(std::isfinite(clampedChasePose.x) &&
           std::isfinite(clampedChasePose.y) &&
           std::isfinite(clampedChasePose.z));

    const auto invalidChasePose = tunrun::thirdPersonCameraPose(
        std::numeric_limits<float>::quiet_NaN(), 0.0F,
        0.0F, 0.0F, std::numeric_limits<float>::infinity(),
        0.0F, 0.0F, std::numeric_limits<float>::quiet_NaN());
    assert(std::isfinite(invalidChasePose.x) &&
           std::isfinite(invalidChasePose.y) &&
           std::isfinite(invalidChasePose.z));

    assert(!tunrun::isModeImplemented(tunrun::GameModeChoice::Campaign));
    assert(!tunrun::isModeImplemented(tunrun::GameModeChoice::Endless));
    assert(tunrun::isModeImplemented(tunrun::GameModeChoice::CustomSeedRun));
    assert(tunrun::isModeImplemented(tunrun::GameModeChoice::PracticePreview));
    assert(!tunrun::isModeImplemented(tunrun::GameModeChoice::Back));
    using tunrun::RunResultNotice;
    assert(tunrun::classifyRunResultNotice(false, false, false) == RunResultNotice::None);
    assert(tunrun::classifyRunResultNotice(true, true, true) == RunResultNotice::SaveFailure);
    assert(tunrun::classifyRunResultNotice(false, true, true) == RunResultNotice::NewScoreAndCombo);
    assert(tunrun::classifyRunResultNotice(false, true, false) == RunResultNotice::NewScore);
    assert(tunrun::classifyRunResultNotice(false, false, true) == RunResultNotice::NewCombo);
    assert(std::string(tunrun::runResultNoticeText(RunResultNotice::SaveFailure)).find(
        "MAY NOT BE SAVED") != std::string::npos);
    assert(tunrun::runResultNoticeText(RunResultNotice::None) == nullptr);
    assert(std::string(tunrun::modeUnavailableMessage(
        tunrun::GameModeChoice::Campaign)).find("not implemented") != std::string::npos);
    assert(std::string(tunrun::modeUnavailableMessage(
        tunrun::GameModeChoice::Endless)).find("not a separate mode") != std::string::npos);

    assert(tunrun::targetFpsForRefreshRate(0) == tunrun::kDefaultTargetFps);
    assert(tunrun::targetFpsForRefreshRate(15) == tunrun::kDefaultTargetFps);
    assert(tunrun::targetFpsForRefreshRate(30) == 30);
    assert(tunrun::targetFpsForRefreshRate(60) == 60);
    assert(tunrun::targetFpsForRefreshRate(144) == 144);
    assert(tunrun::targetFpsForRefreshRate(180) == 180);
    assert(tunrun::targetFpsForRefreshRate(240) == 240);
    assert(tunrun::targetFpsForRefreshRate(360) == tunrun::kMaximumTargetFps);

    assert(!tunrun::triggerPressed(-1.0F));
    assert(!tunrun::triggerPressed(0.0F));
    assert(!tunrun::triggerPressed(-0.01F));
    assert(tunrun::triggerPressed(0.01F));
    assert(tunrun::triggerPressed(1.0F));
    assert(!tunrun::triggerPressed(std::numeric_limits<float>::quiet_NaN()));
    assert(!tunrun::triggerPressed(std::numeric_limits<float>::infinity()));

    ButtonEdge edge;
    assert(!edge.update(false));
    assert(edge.update(true));
    assert(!edge.update(true));
    assert(!edge.update(true));
    assert(!edge.update(false));
    assert(edge.update(true));
    edge.reset();
    assert(!edge.update(false));
    assert(edge.update(true));

    ScreenStack stack;
    assert(stack.current() == Screen::MainMenu);
    assert(stack.size() == 1U);
    assert(!stack.pop());
    assert(tunrun::shouldAdvanceRunClock(stack));
    stack.push(Screen::Preview);
    assert(tunrun::shouldAdvanceRunClock(stack));
    stack.push(Screen::Pause);
    assert(!tunrun::shouldAdvanceRunClock(stack));
    stack.push(Screen::Controls);
    assert(!tunrun::shouldAdvanceRunClock(stack));
    stack.pop();
    stack.push(Screen::Settings);
    assert(!tunrun::shouldAdvanceRunClock(stack));
    stack.pop();
    stack.pop();
    assert(tunrun::shouldAdvanceRunClock(stack));
    stack.replace(Screen::Crash);
    assert(!tunrun::shouldAdvanceRunClock(stack));
    stack.replace(Screen::Preview);
    assert(tunrun::shouldAdvanceRunClock(stack));
    stack.reset();

    assert(tunrun::shouldDrawDashStreaks(false, 0.1F));
    assert(!tunrun::shouldDrawDashStreaks(true, 0.1F));
    assert(!tunrun::shouldDrawDashStreaks(false, 0.0F));
    assert(!tunrun::shouldDrawDashStreaks(false, -0.1F));
    assert(!tunrun::shouldDrawDashStreaks(false,
        std::numeric_limits<float>::quiet_NaN()));
    stack.push(Screen::Records);
    assert(stack.current() == Screen::Records);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.push(Screen::Pause);
    stack.push(Screen::Controls);
    assert(stack.current() == Screen::Controls);
    assert(stack.contains(Screen::Pause) && stack.contains(Screen::Controls));
    assert(stack.pop() && stack.current() == Screen::Pause);
    stack.push(Screen::RunConfirm);
    assert(stack.current() == Screen::RunConfirm);
    assert(stack.contains(Screen::Pause) && stack.contains(Screen::RunConfirm));
    assert(stack.pop() && stack.current() == Screen::Pause);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    assert(!stack.contains(Screen::Pause));
    stack.push(Screen::Preview);
    stack.push(Screen::Pause);
    stack.push(Screen::Settings);
    stack.push(Screen::SettingsResetConfirm);
    assert(stack.current() == Screen::SettingsResetConfirm);
    assert(stack.pop() && stack.current() == Screen::Settings);
    assert(stack.pop() && stack.current() == Screen::Pause);
    assert(stack.pop() && stack.current() == Screen::Preview);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.push(Screen::Settings);
    stack.replace(Screen::Credits);
    assert(stack.current() == Screen::Credits);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.reset();
    assert(stack.current() == Screen::MainMenu && stack.size() == 1U);
    stack.push(Screen::SeedLab); stack.push(Screen::SeedEntry);
    assert(stack.current()==Screen::SeedEntry);
    assert(stack.pop() && stack.current()==Screen::SeedLab);
    assert(stack.pop() && stack.current()==Screen::MainMenu);
    const auto literal=tunrun::parseSeedText("0x0123456789abcdef");
    assert(literal.valid && literal.hexadecimal && literal.seed==0x0123456789ABCDEFULL);
    const auto bare=tunrun::parseSeedText("0123456789ABCDEF");
    assert(bare.valid && bare.seed==literal.seed);
    assert(tunrun::formatHexSeed(literal.seed) == "0x0123456789abcdef");
    assert(tunrun::formatHexSeed(0U) == "0x0000000000000000");
    assert(tunrun::formatHexSeed(std::numeric_limits<std::uint64_t>::max()) ==
           "0xffffffffffffffff");
    const auto copiedSeedRoundTrip = tunrun::parseSeedText(tunrun::formatHexSeed(literal.seed));
    assert(copiedSeedRoundTrip.valid && copiedSeedRoundTrip.hexadecimal &&
           copiedSeedRoundTrip.seed == literal.seed);
    const auto textA=tunrun::parseSeedText("  White   Tunnel_7 ");
    const auto textB=tunrun::parseSeedText("white tunnel_7");
    assert(textA.valid && textB.valid && textA.seed==textB.seed && !textA.hexadecimal);
    assert(!tunrun::parseSeedText("").valid && !tunrun::parseSeedText("0x").valid);
    assert(!tunrun::parseSeedText("bad@seed").valid);
    assert(!tunrun::parseSeedText(std::string(65U,'x')).valid);

    float mouseX = 0.0F;
    float mouseY = 0.0F;
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{100.0F, -50.0F}, 0.004F);
    assert(std::abs(mouseX - 0.4F) < 0.0001F);
    assert(std::abs(mouseY - 0.2F) < 0.0001F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{5000.0F, -5000.0F}, 0.004F);
    assert(mouseX == 3.1F && mouseY == 3.1F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{1.0F, 1.0F}, -1.0F);
    assert(mouseX == 3.1F && mouseY == 3.1F);
    assert(mouseTargetSteering(1.0F, 0.0F, 0.0F) > 0.0F);
    assert(mouseTargetSteering(-1.0F, 0.0F, 0.0F) < 0.0F);
    assert(mouseTargetSteering(0.0F, 0.0F, 1.0F) < 0.0F);
    assert(mouseTargetSteering(0.0F, 0.0F, 0.0F) == 0.0F);
    assert(mouseTargetSteering(std::numeric_limits<float>::quiet_NaN(),
        0.0F, 0.0F) == 0.0F);

    float aimX = 3.1F;
    float aimY = 3.1F;
    assert(clampMouseTargetToRadius(aimX, aimY, 4.0F));
    assert(std::abs(std::hypot(aimX, aimY) - 4.0F) < 0.001F);
    assert(!clampMouseTargetToRadius(aimX, aimY, 4.5F));
    float invalidAimX = std::numeric_limits<float>::quiet_NaN();
    float invalidAimY = std::numeric_limits<float>::infinity();
    (void)clampMouseTargetToRadius(invalidAimX, invalidAimY, 3.0F);
    assert(std::isfinite(invalidAimX) && std::isfinite(invalidAimY));
    float zeroAimX = 1.0F, zeroAimY = -1.0F;
    assert(clampMouseTargetToRadius(zeroAimX, zeroAimY, 0.0F));
    assert(zeroAimX == 0.0F && zeroAimY == 0.0F);

    assert(adjustMouseSensitivity(kMouseSensitivityDefault, 1) ==
           kMouseSensitivityDefault + kMouseSensitivityStep);
    assert(adjustMouseSensitivity(kMouseSensitivityDefault, -1) ==
           kMouseSensitivityDefault - kMouseSensitivityStep);
    assert(adjustMouseSensitivity(kMouseSensitivityMin, -1) ==
           kMouseSensitivityMin);
    assert(adjustMouseSensitivity(kMouseSensitivityMax, 1) ==
           kMouseSensitivityMax);
    assert(adjustMouseSensitivity(
        std::numeric_limits<float>::quiet_NaN(), 0) == kMouseSensitivityDefault);
    assert(adjustMouseSensitivity(-100.0F, 1) >= kMouseSensitivityMin);
    assert(adjustMouseSensitivity(100.0F, -1) <= kMouseSensitivityMax);
    assert(mouseSensitivityFromSlider(0.0F) == kMouseSensitivityMin);
    assert(mouseSensitivityFromSlider(1.0F) == kMouseSensitivityMax);
    assert(mouseSensitivityFromSlider(-2.0F) == kMouseSensitivityMin);
    assert(mouseSensitivityFromSlider(2.0F) == kMouseSensitivityMax);
    assert(mouseSensitivityFromSlider(
        std::numeric_limits<float>::quiet_NaN()) == kMouseSensitivityDefault);
    assert(mouseSensitivitySliderPosition(kMouseSensitivityMin) == 0.0F);
    assert(mouseSensitivitySliderPosition(kMouseSensitivityMax) == 1.0F);
    const float defaultSliderPosition = mouseSensitivitySliderPosition(
        kMouseSensitivityDefault);
    assert(defaultSliderPosition > 0.5F && defaultSliderPosition < 0.7F);
    assert(mouseSensitivityFromSlider(defaultSliderPosition) ==
           kMouseSensitivityDefault);
    const float lowerSensitivity = mouseSensitivityFromSlider(0.25F);
    const float middleSensitivity = mouseSensitivityFromSlider(0.5F);
    const float higherSensitivity = mouseSensitivityFromSlider(0.75F);
    assert(lowerSensitivity < middleSensitivity && middleSensitivity < higherSensitivity);

    constexpr std::uint64_t seed = 0x123456789ABCDEF0ULL;
    const auto section = tunrun::sampleCourse(seed, 1.25);
    assert(section.radius >= tunrun::kCourseMinRadius && section.radius <= tunrun::kCourseMaxRadius);
    const tunrun::TunnelCrossSection centredSection{0.0F, 0.0F, section.radius, section.twist};
    assert(!tunrun::collidesWithTunnelWall(0.0F, 0.0F, centredSection));
    const float safeRadius = section.radius - tunrun::kCraftCollisionRadius;
    assert(tunrun::collidesWithTunnelWall(safeRadius + 0.01F, 0.0F, centredSection));

    assert(tunrun::courseHash(seed) == tunrun::courseHash(seed));
    assert(tunrun::courseHash(seed) != tunrun::courseHash(seed + 1U));
    // Emit a one-line cross-compiler fingerprint while fixed expected values
    // are established. The exact same fixture runs under MSVC and GCC.
    std::cout << "TUNRUN_HASH_FIXTURE seed=" << seed
              << " course=" << tunrun::courseHash(seed)
              << " gates=" << tunrun::obstacleHash(seed)
              << " hazards=" << tunrun::hazardHash(seed)
              << " rewards=" << tunrun::rewardHash(seed) << '\n';
    const auto gate = tunrun::gateAt(seed, 3U);
    tunrun::RunScore score;
    const auto perfectScore = tunrun::awardGatePass(
        score, gate, gate.offsetX, gate.offsetY);
    assert(perfectScore.accepted && perfectScore.clean);
    assert(perfectScore.multiplierTenths == 10U);
    assert(perfectScore.points > tunrun::baseGateScore(gate.kind));
    assert(score.total == perfectScore.points);
    assert(score.gatesPassed == 1U && score.cleanPasses == 1U);
    assert(score.combo == 1U && score.bestCombo == 1U);
    const auto secondScore = tunrun::awardGatePass(
        score, gate, gate.offsetX, gate.offsetY);
    assert(secondScore.accepted && secondScore.multiplierTenths == 11U);
    assert(score.combo == 2U && score.bestCombo == 2U);
    tunrun::RunScore cappedComboScore;
    cappedComboScore.combo = 40U;
    const auto cappedMultiplier = tunrun::awardGatePass(
        cappedComboScore, gate, gate.offsetX, gate.offsetY);
    assert(cappedMultiplier.accepted && cappedMultiplier.multiplierTenths == 50U);
    const auto scoreBeforeReject = score.total;
    const auto missedGateScore = tunrun::awardGatePass(
        score, gate, gate.offsetX + gate.apertureRadius, gate.offsetY);
    assert(!missedGateScore.accepted && score.total == scoreBeforeReject);
    const auto invalidScore = tunrun::awardGatePass(
        score, gate, std::numeric_limits<float>::quiet_NaN(), gate.offsetY);
    assert(!invalidScore.accepted && score.total == scoreBeforeReject);
    tunrun::breakScoreCombo(score);
    assert(score.combo == 0U && score.bestCombo == 2U);
    assert(tunrun::saturatingScoreAdd(tunrun::kScoreCap - 2U, 10U) ==
           tunrun::kScoreCap);
    assert(tunrun::saturatingScoreAdd(tunrun::kScoreCap, 1U) ==
           tunrun::kScoreCap);
    std::uint64_t careerBestScore = 900U;
    std::uint64_t careerBestCombo = 4U;
    tunrun::RunScore recordRun;
    recordRun.total = 1200U;
    recordRun.bestCombo = 7U;
    const auto bothRecords = tunrun::updateCareerBests(
        recordRun, careerBestScore, careerBestCombo);
    assert(bothRecords.scoreImproved && bothRecords.comboImproved);
    assert(careerBestScore == 1200U && careerBestCombo == 7U);
    const auto tiedRecords = tunrun::updateCareerBests(
        recordRun, careerBestScore, careerBestCombo);
    assert(!tiedRecords.scoreImproved && !tiedRecords.comboImproved);
    recordRun.total = 1150U;
    recordRun.bestCombo = 8U;
    const auto comboOnly = tunrun::updateCareerBests(
        recordRun, careerBestScore, careerBestCombo);
    assert(!comboOnly.scoreImproved && comboOnly.comboImproved);
    assert(careerBestScore == 1200U && careerBestCombo == 8U);
    assert(!tunrun::collidesWithGate(gate.offsetX, gate.offsetY, gate));
    assert(tunrun::collidesWithGate(gate.offsetX + gate.apertureRadius, gate.offsetY, gate));
    assert(tunrun::crossesGatePlane(gate.distance - 0.1, gate.distance + 0.1, gate));
    assert(!tunrun::crossesGatePlane(gate.distance + 0.5, gate.distance + 0.7, gate));
    const auto gateSection = tunrun::sampleCourse(seed, gate.distance);
    const auto gateBeforeSection = tunrun::sampleCourse(seed, gate.distance - 0.7);
    const auto gateAfterSection = tunrun::sampleCourse(seed, gate.distance + 0.7);
    const float centeredPreviousX = gate.offsetX + gateSection.centerX -
                                    gateBeforeSection.centerX;
    const float centeredPreviousY = gate.offsetY + gateSection.centerY -
                                    gateBeforeSection.centerY;
    const float centeredCurrentX = gate.offsetX + gateSection.centerX -
                                   gateAfterSection.centerX;
    const float centeredCurrentY = gate.offsetY + gateSection.centerY -
                                   gateAfterSection.centerY;
    assert(!tunrun::collidesWithGateAtCourseCrossing(
        seed, centeredPreviousX, centeredPreviousY, gate.distance - 0.7,
        centeredCurrentX, centeredCurrentY, gate.distance + 0.7, gate));
    assert(!tunrun::collidesWithGateAtCourseCrossing(
        seed, gate.offsetX, gate.offsetY, gate.distance + 0.1,
        gate.offsetX, gate.offsetY, gate.distance + 0.2, gate));
    const auto scoredCrossing = tunrun::gatePointAtCourseCrossing(
        seed, centeredPreviousX, centeredPreviousY, gate.distance - 0.7,
        centeredCurrentX, centeredCurrentY, gate.distance + 0.7, gate);
    assert(scoredCrossing.crossedPlane && scoredCrossing.valid);
    assert(std::abs(scoredCrossing.x - gate.offsetX) < 0.001F);
    assert(std::abs(scoredCrossing.y - gate.offsetY) < 0.001F);
    assert(!tunrun::collidesWithGateAtCrossingPoint(scoredCrossing, gate));

    auto outsideCrossing = scoredCrossing;
    outsideCrossing.x = gate.offsetX + gate.apertureRadius;
    assert(tunrun::collidesWithGateAtCrossingPoint(outsideCrossing, gate));
    tunrun::GateCrossingPoint notCrossed;
    assert(!tunrun::collidesWithGateAtCrossingPoint(notCrossed, gate));
    tunrun::GateCrossingPoint invalidCrossing;
    invalidCrossing.crossedPlane = true;
    assert(tunrun::collidesWithGateAtCrossingPoint(invalidCrossing, gate));

    // Reward placement is seeded, separately versioned, and independent from
    // the obstacle layout. Swept plane checks prevent missed high-speed pickups.
    const auto firstReward = tunrun::rewardAt(seed, 0U);
    const auto repeatedReward = tunrun::rewardAt(seed, 0U);
    assert(firstReward.index == repeatedReward.index);
    assert(firstReward.distance == repeatedReward.distance);
    assert(firstReward.offsetX == repeatedReward.offsetX);
    assert(firstReward.offsetY == repeatedReward.offsetY);
    assert(firstReward.kind == tunrun::RewardKind::AetherShard);
    assert(firstReward.shardValue >= 4U && firstReward.shardValue <= 8U);
    assert(tunrun::rewardAt(seed, 7U).kind == tunrun::RewardKind::SingularityCore);
    assert(tunrun::rewardAt(seed, 7U).shardValue == 0U);
    assert(tunrun::rewardHash(seed) == tunrun::rewardHash(seed));
    assert(tunrun::rewardHash(seed) != tunrun::rewardHash(seed + 1U));
    assert(tunrun::rewardHash(seed, 0U) == 0U);
    assert(tunrun::collectsReward(firstReward.offsetX, firstReward.offsetY, firstReward));
    assert(!tunrun::collectsReward(firstReward.offsetX + tunrun::kRewardPickupRadius * 2.0F,
                                   firstReward.offsetY, firstReward));
    assert(tunrun::crossesRewardPlane(firstReward.distance - 0.1,
                                      firstReward.distance + 0.1, firstReward));
    assert(!tunrun::crossesRewardPlane(firstReward.distance + 0.1,
                                       firstReward.distance - 0.1, firstReward));
    assert(!tunrun::crossesRewardPlane(firstReward.distance + 0.1,
                                       firstReward.distance + 0.2, firstReward));
    assert(!tunrun::collectsReward(std::numeric_limits<float>::quiet_NaN(),
                                   firstReward.offsetY, firstReward));
    const auto rewardSection = tunrun::sampleCourse(seed, firstReward.distance);
    const auto rewardBeforeSection = tunrun::sampleCourse(seed, firstReward.distance - 0.7);
    const auto rewardAfterSection = tunrun::sampleCourse(seed, firstReward.distance + 0.7);
    const float rewardPreviousX = firstReward.offsetX + rewardSection.centerX -
                                  rewardBeforeSection.centerX;
    const float rewardPreviousY = firstReward.offsetY + rewardSection.centerY -
                                  rewardBeforeSection.centerY;
    const float rewardCurrentX = firstReward.offsetX + rewardSection.centerX -
                                 rewardAfterSection.centerX;
    const float rewardCurrentY = firstReward.offsetY + rewardSection.centerY -
                                 rewardAfterSection.centerY;
    assert(tunrun::collectsRewardAtCourseCrossing(
        seed, rewardPreviousX, rewardPreviousY, firstReward.distance - 0.7,
        rewardCurrentX, rewardCurrentY, firstReward.distance + 0.7, firstReward));
    assert(!tunrun::collectsRewardAtCourseCrossing(
        seed, rewardPreviousX, rewardPreviousY, firstReward.distance + 0.1,
        rewardCurrentX, rewardCurrentY, firstReward.distance + 0.7, firstReward));
    assert(!tunrun::collectsRewardAtCourseCrossing(
        seed, rewardPreviousX + 3.0F, rewardPreviousY, firstReward.distance - 0.7,
        rewardCurrentX + 3.0F, rewardCurrentY, firstReward.distance + 0.7, firstReward));
    for (std::uint32_t rewardIndex = 0U; rewardIndex < 512U; ++rewardIndex) {
        const auto reward = tunrun::rewardAt(seed, rewardIndex);
        assert(reward.index == rewardIndex);
        assert(reward.distance == tunrun::kRewardStartDistance +
                                  static_cast<double>(rewardIndex) * tunrun::kRewardSpacing);
        assert(std::abs(reward.offsetX) <= 1.45F);
        assert(std::abs(reward.offsetY) <= 1.45F);
        if (reward.kind == tunrun::RewardKind::AetherShard) {
            assert(reward.shardValue >= 4U && reward.shardValue <= 8U);
        } else {
            assert(reward.shardValue == 0U);
        }
    }

    // Moving hazards are seeded and their positions are deterministic at a
    // given run-clock time; plane crossing and contact reject invalid inputs.
    const auto hazard = tunrun::hazardAt(seed, 0U);
    const auto hazardAgain = tunrun::hazardAt(seed, 0U);
    assert(hazard.index == hazardAgain.index);
    assert(hazard.distance == hazardAgain.distance);
    assert(hazard.baseX == hazardAgain.baseX);
    assert(hazard.amplitudeX == hazardAgain.amplitudeX);
    assert(hazard.phaseX == hazardAgain.phaseX);
    assert(hazard.frequency >= 0.95F && hazard.frequency <= 1.80F);
    assert(tunrun::kHazardGeneratorVersion == 2U);
    assert(tunrun::hazardVisualFamilyAt(seed, 0U) ==
           tunrun::hazardVisualFamilyAt(seed, 0U));
    bool seenOrbitalMine = false, seenPrismMine = false;
    bool seenRotorMine = false, seenCrossMine = false;
    for (std::uint32_t index = 0U; index < 512U; ++index) {
        switch (tunrun::hazardVisualFamilyAt(seed, index)) {
        case tunrun::HazardVisualFamily::Orbital: seenOrbitalMine = true; break;
        case tunrun::HazardVisualFamily::Prism: seenPrismMine = true; break;
        case tunrun::HazardVisualFamily::Rotor: seenRotorMine = true; break;
        case tunrun::HazardVisualFamily::Cross: seenCrossMine = true; break;
        }
    }
    assert(seenOrbitalMine && seenPrismMine && seenRotorMine && seenCrossMine);
    assert(std::string_view(tunrun::hazardVisualFamilyName(
        tunrun::hazardVisualFamilyAt(seed, 0U))).size() > 0U);
    const auto hazardCenter = tunrun::hazardCenterAt(hazard, 1.25);
    const auto hazardCenterAgain = tunrun::hazardCenterAt(hazardAgain, 1.25);
    assert(hazardCenter.x == hazardCenterAgain.x &&
           hazardCenter.y == hazardCenterAgain.y);

    const auto cue = tunrun::hazardHudCueAt(seed, 0.0, 1.25, 0.0F, 0.0F);
    const auto repeatedCue = tunrun::hazardHudCueAt(seed, 0.0, 1.25, 0.0F, 0.0F);
    assert(cue.valid && cue.hazardIndex == 0U);
    assert(cue.distanceAhead == hazard.distance);
    assert(cue.offsetX == repeatedCue.offsetX && cue.offsetY == repeatedCue.offsetY);
    const auto playerSectionForCue = tunrun::sampleCourse(seed, 0.0);
    const auto hazardSectionForCue = tunrun::sampleCourse(seed, hazard.distance);
    assert(std::abs(cue.offsetX -
        (hazardSectionForCue.centerX - playerSectionForCue.centerX + hazardCenter.x)) < 0.0001F);
    assert(std::abs(cue.offsetY -
        (hazardSectionForCue.centerY - playerSectionForCue.centerY + hazardCenter.y)) < 0.0001F);
    const auto shiftedCue = tunrun::hazardHudCueAt(seed, 0.0, 1.25, 0.5F, -0.25F);
    assert(shiftedCue.valid && std::abs(shiftedCue.offsetX - (cue.offsetX - 0.5F)) < 0.0001F);
    assert(std::abs(shiftedCue.offsetY - (cue.offsetY + 0.25F)) < 0.0001F);
    const auto nextCue = tunrun::hazardHudCueAt(
        seed, hazard.distance + 0.01, 1.25, 0.0F, 0.0F);
    assert(nextCue.valid && nextCue.hazardIndex == 1U);
    assert(nextCue.distanceAhead > 0.0);
    assert(!tunrun::hazardHudCueAt(
        seed, std::numeric_limits<double>::quiet_NaN(), 1.25, 0.0F, 0.0F).valid);
    assert(!tunrun::hazardHudCueAt(
        seed, 0.0, std::numeric_limits<double>::infinity(), 0.0F, 0.0F).valid);
    assert(tunrun::collidesWithHazard(hazardCenter.x, hazardCenter.y,
                                      hazard, 1.25));
    assert(!tunrun::collidesWithHazard(hazardCenter.x + hazard.radius +
                                      tunrun::kHazardCraftCollisionRadius + 0.2F,
                                      hazardCenter.y, hazard, 1.25));
    assert(tunrun::crossesHazardPlane(hazard.distance - 0.1,
                                      hazard.distance + 0.1, hazard));
    assert(!tunrun::crossesHazardPlane(hazard.distance + 0.1,
                                       hazard.distance - 0.1, hazard));
    assert(!tunrun::crossesHazardPlane(hazard.distance + 0.1,
                                       hazard.distance + 0.2, hazard));
    assert(!tunrun::crossesHazardPlane(
        std::numeric_limits<double>::quiet_NaN(), hazard.distance + 0.1, hazard));
    const auto hazardAtTime = tunrun::hazardCenterAt(hazard, 1.0);
    const float hazardCombinedRadius = hazard.radius +
                                       tunrun::kHazardCraftCollisionRadius;
    // Contact can happen at the near edge before the craft reaches the
    // hazard center plane; the swept check must not miss it.
    assert(tunrun::sweptCollidesWithHazard(
        seed, hazardAtTime.x + hazardCombinedRadius - 0.15F, hazardAtTime.y,
        hazard.distance - 0.8, 1.0,
        hazardAtTime.x + hazardCombinedRadius - 0.15F, hazardAtTime.y,
        hazard.distance - 0.2, 1.01, hazard));
    assert(!tunrun::sweptCollidesWithHazard(
        seed, hazardAtTime.x + hazardCombinedRadius + 0.2F, hazardAtTime.y,
        hazard.distance - 0.8, 1.0,
        hazardAtTime.x + hazardCombinedRadius + 0.2F, hazardAtTime.y,
        hazard.distance + 0.8, 1.02, hazard));
    // A render frame can have no fixed-step forward-distance advance. Mine
    // contact still has to be checked while the hazard clock advances.
    const auto stationaryStart = tunrun::hazardCenterAt(hazard, 1.0);
    const auto stationaryEnd = tunrun::hazardCenterAt(hazard, 1.02);
    assert(tunrun::sweptCollidesWithHazard(
        seed, stationaryStart.x, stationaryStart.y, hazard.distance, 1.0,
        stationaryEnd.x, stationaryEnd.y, hazard.distance, 1.02, hazard));
    assert(tunrun::sweptCollidesWithHazard(
        seed, stationaryStart.x, stationaryStart.y, hazard.distance, 1.0,
        stationaryStart.x, stationaryStart.y, hazard.distance, 1.0, hazard));
    assert(!tunrun::sweptCollidesWithHazard(
        seed, hazardAtTime.x, hazardAtTime.y,
        hazard.distance + 0.8, 1.01,
        hazardAtTime.x, hazardAtTime.y,
        hazard.distance - 0.8, 1.0, hazard));
    assert(tunrun::sweptCollidesWithHazard(
        seed, std::numeric_limits<float>::quiet_NaN(), 0.0F,
        hazard.distance - 1.0, 1.0,
        0.0F, 0.0F, hazard.distance + 1.0, 1.1, hazard));

    // Curved-centerline regression using the same course frame as the renderer.
    const auto hazardSection = tunrun::sampleCourse(seed, hazard.distance);
    const auto hazardBeforeSection = tunrun::sampleCourse(seed, hazard.distance - 0.7);
    const auto hazardAfterSection = tunrun::sampleCourse(seed, hazard.distance + 0.7);
    const auto hazardBeforeCenter = tunrun::hazardCenterAt(hazard, 2.0);
    const auto hazardAfterCenter = tunrun::hazardCenterAt(hazard, 2.02);
    const float safeHazardPreviousX = hazardSection.centerX + hazardBeforeCenter.x -
                                      hazardBeforeSection.centerX;
    const float safeHazardPreviousY = hazardSection.centerY + hazardBeforeCenter.y -
                                      hazardBeforeSection.centerY;
    const float safeHazardCurrentX = hazardSection.centerX + hazardAfterCenter.x -
                                     hazardAfterSection.centerX;
    const float safeHazardCurrentY = hazardSection.centerY + hazardAfterCenter.y -
                                     hazardAfterSection.centerY;
    assert(tunrun::sweptCollidesWithHazard(
        seed, safeHazardPreviousX, safeHazardPreviousY, hazard.distance - 0.7, 2.0,
        safeHazardCurrentX, safeHazardCurrentY, hazard.distance + 0.7, 2.02, hazard));

    // Regression batch: several derived seeds must keep hazards within the
    // declared motion envelope and outside gate reaction windows.
    for (std::uint64_t seedIndex = 0U; seedIndex < 24U; ++seedIndex) {
        const std::uint64_t batchSeed = tunrun::deriveCourseSeed(seed, seedIndex);
        const auto batchValidation = tunrun::validateHazardSet(batchSeed, 128U);
        assert(batchValidation.valid);
        assert(batchValidation.hazardsChecked == 128U);
        assert(batchValidation.minimumGateSeparation >=
               tunrun::kHazardMinimumGateSeparation);
        assert(tunrun::hazardHash(batchSeed) != tunrun::hazardHash(seed));
    }
    assert(tunrun::hazardHash(seed) == tunrun::hazardHash(seed));
    assert(tunrun::hazardHash(seed) != tunrun::hazardHash(seed + 1U));
    assert(tunrun::hazardHash(seed, 0U) == 0U);
    const auto hazardValidation = tunrun::validateHazardSet(seed, 512U);
    assert(hazardValidation.valid && hazardValidation.hazardsChecked == 512U);
    assert(hazardValidation.maximumHorizontalExtent <= 2.401F);
    assert(hazardValidation.maximumVerticalExtent <= 1.551F);
    assert(hazardValidation.minimumGateSeparation >=
           tunrun::kHazardMinimumGateSeparation);
    assert(!tunrun::validateHazardSet(seed, 0U).valid);
    assert(!tunrun::validateHazardSet(seed, 10001U).valid);
    assert(tunrun::collidesWithHazard(std::numeric_limits<float>::quiet_NaN(),
                                      0.0F, hazard, 0.0));

    // Obstacle layout has its own deterministic versioned fingerprint.
    assert(tunrun::obstacleHash(seed) == tunrun::obstacleHash(seed));
    assert(tunrun::obstacleHash(seed) != tunrun::obstacleHash(seed + 1U));
    assert(tunrun::obstacleHash(seed, 0U) == 0U);
    const auto mixedGateValidation = tunrun::validateObstacleSet(seed, 512U);
    assert(mixedGateValidation.valid && mixedGateValidation.gatesChecked == 512U);
    assert(mixedGateValidation.standardGates + mixedGateValidation.precisionGates +
           mixedGateValidation.offsetGates + mixedGateValidation.wideGates ==
           mixedGateValidation.gatesChecked);
    bool seenStandard = false, seenPrecision = false, seenOffset = false, seenWide = false;
    for (std::uint32_t index = 0U; index < 512U; ++index) {
        const auto generatedGate = tunrun::gateAt(seed, index);
        assert(!tunrun::collidesWithGate(generatedGate.offsetX, generatedGate.offsetY,
                                         generatedGate));
        assert(tunrun::collidesWithGate(generatedGate.offsetX + generatedGate.apertureRadius,
                                        generatedGate.offsetY, generatedGate));
        switch (generatedGate.kind) {
        case tunrun::GateKind::Standard: seenStandard = true; break;
        case tunrun::GateKind::Precision: seenPrecision = true; break;
        case tunrun::GateKind::Offset: seenOffset = true; break;
        case tunrun::GateKind::Wide: seenWide = true; break;
        }
    }
    assert(seenStandard && seenPrecision && seenOffset && seenWide);
    assert(tunrun::validateObstacleSet(seed, 64U).valid);

    // The ship catalog is shared by the hangar, renderer, and physics model.
    assert(tunrun::kShipCatalog.size() == 8U);
    assert(tunrun::shipDefinition(1U).aetherShardCost == 8000U);
    assert(tunrun::shipDefinition(2U).aetherShardCost == 15000U);
    assert(tunrun::shipDefinition(3U).aetherShardCost == 25000U);
    assert(tunrun::shipDefinition(4U).aetherShardCost == 38000U);
    assert(tunrun::shipDefinition(5U).singularityCoreCost == 200U);
    assert(tunrun::shipDefinition(6U).singularityCoreCost == 400U);
    assert(tunrun::shipDefinition(7U).singularityCoreCost == 800U);
    for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
        const auto& ship = tunrun::shipDefinition(shipId);
        assert(ship.name != nullptr && std::string_view(ship.name).size() > 0U);
        assert(ship.speedMultiplier > 0.0F);
        assert(ship.accelerationMultiplier > 0.0F);
        assert(ship.boostDrainMultiplier > 0.0F);
        assert(ship.energyRegenerationMultiplier > 0.0F);
        for (std::uint32_t other = 0U; other < shipId; ++other) {
            assert(std::string_view(ship.name) !=
                   std::string_view(tunrun::shipDefinition(other).name));
        }
    }
    assert(&tunrun::shipDefinition(999U) ==
           &tunrun::shipDefinition(tunrun::kStarterShipId));

    // Check adjacent gate transitions against every ship profile at maximum
    // forward (boost) speed, using each ship's actual handling parameters.
    for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
        const auto reachability = tunrun::validateGateReachability(seed, 512U, shipId);
        assert(reachability.valid);
        assert(reachability.gatesChecked == 512U);
        assert(reachability.transitionsChecked == 511U);
        assert(reachability.minimumReachableSlack >= -0.0001F);
        assert(reachability.minimumTravelTime > 0.0);
    }
    assert(!tunrun::validateGateReachability(seed, 0U).valid);
    assert(!tunrun::validateGateReachability(seed, 10001U).valid);
    assert(!tunrun::validateGateReachability(seed, 32U, 999U).valid);


    // Route graph policies cover every aim offset in cruise, boost,
    // precision and one-shot dash modes. Dash cannot combine with precision.
    std::array<std::uint32_t, 4U> policyModes{};
    assert(tunrun::kRouteGraphPolicies.size() == 36U);
    for (const auto& policy : tunrun::kRouteGraphPolicies) {
        assert(std::isfinite(policy.aimBiasX) && std::isfinite(policy.aimBiasY));
        assert(!(policy.boost && policy.precision));
        assert(!(policy.dash && policy.precision));
        const std::size_t mode = policy.precision ? 2U :
            policy.dash ? 3U : policy.boost ? 1U : 0U;
        ++policyModes[mode];
    }
    for (const auto count : policyModes) assert(count == 9U);

    // Each 32-state gate beam remains balanced (eight slots per mode), and
    // rotating the omitted offset visits all policies over nine gate indices.
    std::array<bool, 36U> policySeen{};
    for (std::size_t gateIndex = 0U; gateIndex < 9U; ++gateIndex) {
        std::array<std::uint32_t, 4U> modeSlots{};
        std::array<bool, 36U> uniqueInBeam{};
        for (std::size_t slot = 0U; slot < 32U; ++slot) {
            const auto policyIndex =
                tunrun::routeGraphPolicyIndexForSlot(slot, gateIndex);
            assert(policyIndex < tunrun::kRouteGraphPolicies.size());
            assert(!uniqueInBeam[policyIndex]);
            uniqueInBeam[policyIndex] = true;
            policySeen[policyIndex] = true;
            const auto& policy = tunrun::kRouteGraphPolicies[policyIndex];
            const std::size_t mode = policy.precision ? 2U :
                policy.dash ? 3U : policy.boost ? 1U : 0U;
            ++modeSlots[mode];
        }
        for (const auto count : modeSlots) assert(count == 8U);
    }
    for (const bool seen : policySeen) assert(seen);

    // The local mine-avoidance projection must be stable at the singular
    // case where the requested target is exactly on the mine center.
    const auto unchangedAim = tunrun::routeAimOutsideMineEnvelope(
        2.0F, 0.0F, 0.0F, 0.0F, 1.0F, 5.0F,
        0.0F, 0.0F, 0.0F, 0.0F, 0U);
    assert(!unchangedAim.adjusted && unchangedAim.x == 2.0F &&
           unchangedAim.y == 0.0F);
    const auto centeredAim = tunrun::routeAimOutsideMineEnvelope(
        0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 5.0F,
        0.0F, 0.0F, 0.0F, 0.0F, 0U);
    assert(centeredAim.adjusted);
    assert(std::abs(std::hypot(centeredAim.x, centeredAim.y) - 1.0F) < 0.0001F);
    assert(centeredAim.x < 0.0F);
    const auto otherFallback = tunrun::routeAimOutsideMineEnvelope(
        0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 5.0F,
        0.0F, 0.0F, 0.0F, 0.0F, 1U);
    assert(otherFallback.x > 0.0F);
    const auto boundedAim = tunrun::routeAimOutsideMineEnvelope(
        0.2F, 0.0F, 0.0F, 0.0F, 1.0F, 0.6F,
        0.0F, 0.0F, 0.0F, 0.0F, 0U);
    assert(boundedAim.adjusted);
    assert(std::abs(std::hypot(boundedAim.x, boundedAim.y) - 0.6F) < 0.0001F);
    const auto invalidAim = tunrun::routeAimOutsideMineEnvelope(
        std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F, 0.0F, 1.0F, 5.0F,
        0.0F, 0.0F, 0.0F, 0.0F, 0U);
    assert(!invalidAim.adjusted && std::isnan(invalidAim.x));

    // Bounded state-propagating route graph: viable gate crossings branch
    // into multiple target policies while preserving the actual flight state.
    bool sawGraphBeamPruning = false;
    for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
        const auto graph =
            tunrun::validateStateGraphRouteReachability(seed, 12U, shipId);
        if (!graph.valid) {
            std::cerr << "State graph failed: seed=" << seed
                      << " ship=" << shipId
                      << " gate=" << graph.firstFailedGate
                      << " hazardHits=" << graph.hazardCollisionStates
                      << " discarded=" << graph.discardedStates
                      << " reason=" << graph.failure << '\n';
        }
        assert(graph.valid);
        assert(graph.gatesChecked == 12U);
        assert(graph.transitionsChecked == 11U);
        assert(graph.candidateStatesGenerated >= 9U);
        assert(graph.beamPrunedStates <= graph.candidateStatesGenerated);
        assert(graph.hazardChecks > 0U);
        assert(graph.hazardCollisionStates <= graph.discardedStates);
        sawGraphBeamPruning = sawGraphBeamPruning || graph.beamPrunedStates > 0U;
        assert(graph.candidateStatesGenerated >= 32U);
        assert(graph.peakStateCount == 32U);
        assert(graph.maximumPassingStatesAtGate > 0U);
        assert(graph.minimumGateClearance >= 0.0F);
        assert(graph.maximumLateralOffset < tunrun::kFlightLimit);
        assert(graph.simulatedDistance > 0.0);
        if (shipId == 0U) {
            const auto repeatedGraph =
                tunrun::validateStateGraphRouteReachability(seed, 12U, shipId);
            assert(repeatedGraph.valid == graph.valid);
            assert(repeatedGraph.gatesChecked == graph.gatesChecked);
            assert(repeatedGraph.transitionsChecked == graph.transitionsChecked);
            assert(repeatedGraph.simulationSteps == graph.simulationSteps);
            assert(repeatedGraph.candidateStatesGenerated == graph.candidateStatesGenerated);
            assert(repeatedGraph.beamPrunedStates == graph.beamPrunedStates);
            assert(repeatedGraph.hazardChecks == graph.hazardChecks);
            assert(repeatedGraph.hazardCollisionStates == graph.hazardCollisionStates);
            assert(repeatedGraph.discardedStates == graph.discardedStates);
            assert(repeatedGraph.peakStateCount == graph.peakStateCount);
            assert(repeatedGraph.gatesWithMultiplePassingStates ==
                   graph.gatesWithMultiplePassingStates);
            assert(repeatedGraph.maximumPassingStatesAtGate ==
                   graph.maximumPassingStatesAtGate);
            assert(repeatedGraph.minimumGateClearance == graph.minimumGateClearance);
            assert(repeatedGraph.maximumLateralOffset == graph.maximumLateralOffset);
            assert(repeatedGraph.simulatedDistance == graph.simulatedDistance);
        }
    }
    assert(sawGraphBeamPruning);
    assert(!tunrun::validateStateGraphRouteReachability(seed, 0U).valid);
    assert(!tunrun::validateStateGraphRouteReachability(seed, 513U).valid);
    assert(!tunrun::validateStateGraphRouteReachability(seed, 12U, 999U).valid);
    for (std::uint64_t graphSeedIndex = 0U; graphSeedIndex < 32U; ++graphSeedIndex) {
        const auto graphSeed = tunrun::deriveCourseSeed(seed, 300U + graphSeedIndex);
        // Cover each ship against the same derived seeds. Bounded misses are
        // valid diagnostic outcomes; every metric must still replay identically.
        for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
            const auto graph =
                tunrun::validateStateGraphRouteReachability(graphSeed, 8U, shipId);
            assert(graph.gatesChecked <= 8U);
            assert(graph.transitionsChecked <= 7U);
            assert(graph.candidateStatesGenerated >= 9U);
            assert(graph.candidateStatesGenerated >= 32U);
            assert(graph.peakStateCount == 32U);
            assert(graph.beamPrunedStates <= graph.candidateStatesGenerated);
            assert(graph.hazardCollisionStates <= graph.discardedStates);
            assert(graph.failure != nullptr);

            if (graph.valid) {
                assert(graph.gatesChecked == 8U);
                assert(graph.transitionsChecked == 7U);
                assert(graph.minimumGateClearance >= 0.0F);
                assert(graph.bestWitnessMinimumClearance >=
                       graph.minimumGateClearance);
            } else {
                assert(graph.firstFailedGate <= 8U);
            }

            const auto repeat =
                tunrun::validateStateGraphRouteReachability(graphSeed, 8U, shipId);
            assert(repeat.valid == graph.valid);
            assert(repeat.gatesChecked == graph.gatesChecked);
            assert(repeat.transitionsChecked == graph.transitionsChecked);
            assert(repeat.simulationSteps == graph.simulationSteps);
            assert(repeat.candidateStatesGenerated == graph.candidateStatesGenerated);
            assert(repeat.discardedStates == graph.discardedStates);
            assert(repeat.beamPrunedStates == graph.beamPrunedStates);
            assert(repeat.hazardChecks == graph.hazardChecks);
            assert(repeat.hazardCollisionStates == graph.hazardCollisionStates);
            assert(repeat.peakStateCount == graph.peakStateCount);
            assert(repeat.gatesWithMultiplePassingStates ==
                   graph.gatesWithMultiplePassingStates);
            assert(repeat.maximumPassingStatesAtGate ==
                   graph.maximumPassingStatesAtGate);
            assert(repeat.firstFailedGate == graph.firstFailedGate);
            assert(repeat.minimumGateClearance == graph.minimumGateClearance);
            assert(repeat.bestWitnessMinimumClearance ==
                   graph.bestWitnessMinimumClearance);
            assert(repeat.maximumLateralOffset == graph.maximumLateralOffset);
            assert(repeat.simulatedDistance == graph.simulatedDistance);
            assert(std::string_view(repeat.failure) == graph.failure);
        }
    }

    // The fixed-step probe carries lateral position and velocity across gate
    // crossings using the actual gameplay physics for every ship profile.
    for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
        const auto route = tunrun::validateSimulatedRouteReachability(seed, 256U, shipId);
        assert(route.valid);
        assert(route.gatesChecked == 256U);
        assert(route.transitionsChecked == 255U);
        assert(route.simulationSteps > 0U);
        assert(route.minimumGateClearance >= 0.0F);
        assert(route.maximumLateralOffset < tunrun::kFlightLimit);
    }
    assert(!tunrun::validateSimulatedRouteReachability(seed, 0U).valid);
    assert(!tunrun::validateSimulatedRouteReachability(seed, 32U, 999U).valid);

    auto impossiblePrevious = tunrun::gateAt(seed, 0U);
    auto impossibleNext = tunrun::gateAt(seed, 1U);
    impossibleNext.offsetX = 100.0F;
    const auto impossibleTransition = tunrun::evaluateGateTransitionReachability(
        impossiblePrevious, impossibleNext, tunrun::kStarterShipId);
    assert(!impossibleTransition.valid);
    assert(impossibleTransition.failure[0] != '\0');
    impossibleNext = tunrun::gateAt(seed, 1U);
    impossibleNext.distance = impossiblePrevious.distance + 0.5;
    assert(!tunrun::evaluateGateTransitionReachability(
        impossiblePrevious, impossibleNext, tunrun::kStarterShipId).valid);

    assert(tunrun::deriveCourseSeed(seed, 0U) != tunrun::deriveCourseSeed(seed, 1U));
    const auto validation = tunrun::validateCourse(seed, 3600.0);
    assert(validation.valid && validation.samplesChecked > 4000U);
    assert(validation.minimumRadius >= tunrun::kCourseMinRadius);
    assert(validation.maximumRadius <= tunrun::kCourseMaxRadius);
    for (int node = 1; node < 30; ++node) {
        const double seam = static_cast<double>(node) * tunrun::kCourseNodeSpacing;
        const auto before = tunrun::sampleCourse(seed, seam - 0.001);
        const auto after = tunrun::sampleCourse(seed, seam + 0.001);
        assert(std::abs(before.centerX - after.centerX) < 0.01F);
        assert(std::abs(before.centerY - after.centerY) < 0.01F);
        assert(std::abs(before.radius - after.radius) < 0.01F);
        assert(std::abs(before.twist - after.twist) < 0.01F);
    }
    for (std::uint64_t sampleSeed = 0; sampleSeed < 24U; ++sampleSeed) {
        const auto generatedSeed = tunrun::deriveCourseSeed(seed, sampleSeed);
        assert(tunrun::validateCourse(generatedSeed, 1800.0).valid);
        assert(tunrun::validateObstacleSet(generatedSeed, 64U).valid);
        for (std::uint32_t shipId = 0U; shipId < tunrun::kShipCatalog.size(); ++shipId) {
            const auto reachability =
                tunrun::validateGateReachability(generatedSeed, 64U, shipId);
            assert(reachability.valid && reachability.transitionsChecked == 63U);
            // Route simulation is more expensive than the pairwise envelope;
            // run it across a deterministic subset of the broader seed corpus.
            if (sampleSeed < 4U) {
                const auto route =
                    tunrun::validateSimulatedRouteReachability(generatedSeed, 64U, shipId);
                assert(route.valid && route.gatesChecked == 64U);
                assert(route.transitionsChecked == 63U);
                assert(route.minimumGateClearance >= 0.0F);
            }
        }
    }

    tunrun::FlightState normal;
    tunrun::FlightState precision;
    float normalAccumulator = 0.0F;
    float precisionAccumulator = 0.0F;
    const tunrun::FlightInput steer{1.0F, 0.0F, false, false};
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(normal, steer, 1.0F / 60.0F, normalAccumulator);
        tunrun::advanceFlight(precision, tunrun::FlightInput{1.0F, 0.0F, false, true},
                              1.0F / 60.0F, precisionAccumulator);
    }
    assert(normal.x > precision.x);

    tunrun::FlightState boost;
    float boostAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(boost, tunrun::FlightInput{1.0F, 0.0F, true, false},
                              1.0F / 120.0F, boostAccumulator);
    }
    assert(boost.boostEnergy < 100.0F && boost.boostEnergy > 50.0F);

    // W accelerates above the familiar cruise speed; S brakes quickly.
    tunrun::FlightState cruiseFlight;
    tunrun::FlightState acceleratedFlight;
    tunrun::FlightState brakingFlight;
    float cruiseAccumulator = 0.0F, acceleratedAccumulator = 0.0F, brakingAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(cruiseFlight, tunrun::FlightInput{}, 1.0F / 120.0F,
                              cruiseAccumulator);
        tunrun::advanceFlight(acceleratedFlight,
            tunrun::FlightInput{0.0F, 0.0F, false, false,
                                tunrun::kStarterShipId, false, 0.0F, 0.0F, 0.0F, 1.0F},
            1.0F / 120.0F, acceleratedAccumulator);
        tunrun::advanceFlight(brakingFlight,
            tunrun::FlightInput{0.0F, 0.0F, false, false,
                                tunrun::kStarterShipId, false, 0.0F, 0.0F, 0.0F, -1.0F},
            1.0F / 120.0F, brakingAccumulator);
    }
    assert(acceleratedFlight.distance > cruiseFlight.distance);
    assert(brakingFlight.distance < cruiseFlight.distance);
    assert(brakingFlight.forwardSpeed < cruiseFlight.forwardSpeed);
    assert(acceleratedFlight.forwardSpeed > cruiseFlight.forwardSpeed);

    // Braking has priority over a held boost or precision modifier.
    tunrun::FlightState brakingBoostFlight;
    tunrun::FlightState brakingPrecisionFlight;
    float brakingBoostAccumulator = 0.0F, brakingPrecisionAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(brakingBoostFlight,
            tunrun::FlightInput{0.0F, 0.0F, true, false,
                tunrun::kStarterShipId, false, 0.0F, 0.0F, 0.0F, -1.0F},
            1.0F / 120.0F, brakingBoostAccumulator);
        tunrun::advanceFlight(brakingPrecisionFlight,
            tunrun::FlightInput{0.0F, 0.0F, false, true,
                tunrun::kStarterShipId, false, 0.0F, 0.0F, 0.0F, -1.0F},
            1.0F / 120.0F, brakingPrecisionAccumulator);
    }
    assert(brakingBoostFlight.forwardSpeed < 4.0F);
    assert(brakingPrecisionFlight.forwardSpeed < 4.0F);
    assert(brakingBoostFlight.boostEnergy > 99.0F);
    assert(brakingBoostFlight.distance < cruiseFlight.distance);
    assert(brakingPrecisionFlight.distance < cruiseFlight.distance);

    // Dash is an edge-triggered, energy-costed burst. Holding the button
    // does not auto-repeat when the cooldown expires.
    tunrun::FlightState ordinaryFlight;
    tunrun::FlightState dashFlight;
    tunrun::updateFlight(ordinaryFlight,
        tunrun::FlightInput{0.0F, 0.0F, false, false,
                            tunrun::kStarterShipId, false},
        tunrun::kFlightFixedStep);
    tunrun::updateFlight(dashFlight,
        tunrun::FlightInput{0.0F, 0.0F, false, false,
                            tunrun::kStarterShipId, true},
        tunrun::kFlightFixedStep);
    const float dashEnergyAfterActivation = dashFlight.boostEnergy;
    assert(dashFlight.distance > ordinaryFlight.distance);
    assert(dashEnergyAfterActivation < 75.0F);
    assert(dashFlight.dashRemaining > 0.0F);
    assert(dashFlight.dashCooldownRemaining > 0.0F);
    for (int i = 1; i < 240; ++i) {
        tunrun::updateFlight(ordinaryFlight,
            tunrun::FlightInput{0.0F, 0.0F, false, false,
                                tunrun::kStarterShipId, false},
            tunrun::kFlightFixedStep);
        tunrun::updateFlight(dashFlight,
            tunrun::FlightInput{0.0F, 0.0F, false, false,
                                tunrun::kStarterShipId, true},
            tunrun::kFlightFixedStep);
    }
    assert(dashFlight.distance > ordinaryFlight.distance + 1.7F);
    assert(dashFlight.distance < ordinaryFlight.distance + 2.2F);
    assert(dashFlight.dashRemaining == 0.0F);
    assert(dashFlight.dashCooldownRemaining == 0.0F);
    assert(dashFlight.dashButtonWasDown);

    tunrun::updateFlight(dashFlight,
        tunrun::FlightInput{0.0F, 0.0F, false, false,
                            tunrun::kStarterShipId, false},
        tunrun::kFlightFixedStep);
    tunrun::updateFlight(dashFlight,
        tunrun::FlightInput{0.0F, 0.0F, false, false,
                            tunrun::kStarterShipId, true},
        tunrun::kFlightFixedStep);
    assert(dashFlight.dashRemaining > 0.0F);
    assert(dashFlight.dashCooldownRemaining > 0.0F);

    tunrun::FlightState precisionDash;
    tunrun::updateFlight(precisionDash,
        tunrun::FlightInput{0.0F, 0.0F, false, true,
                            tunrun::kStarterShipId, true},
        tunrun::kFlightFixedStep);
    assert(precisionDash.dashRemaining == 0.0F);
    assert(precisionDash.boostEnergy > 99.0F);

    tunrun::FlightState lowEnergyDash;
    lowEnergyDash.boostEnergy = 20.0F;
    tunrun::updateFlight(lowEnergyDash,
        tunrun::FlightInput{0.0F, 0.0F, false, false,
                            tunrun::kStarterShipId, true},
        tunrun::kFlightFixedStep);
    assert(lowEnergyDash.dashRemaining == 0.0F);
    assert(lowEnergyDash.dashCooldownRemaining == 0.0F);
    assert(lowEnergyDash.boostEnergy < 21.0F);

    const auto simulateAtRenderRate = [](int framesPerSecond) {
        tunrun::FlightState state;
        float accumulator = 0.0F;
        const float frameTime = 1.0F / static_cast<float>(framesPerSecond);
        for (int frame = 0; frame < framesPerSecond * 2; ++frame) {
            tunrun::advanceFlight(state, tunrun::FlightInput{0.6F, -0.4F, false, false},
                                  frameTime, accumulator);
        }
        return state;
    };
    const auto at30 = simulateAtRenderRate(30);
    const auto at60 = simulateAtRenderRate(60);
    const auto at120 = simulateAtRenderRate(120);
    assert(std::abs(at30.x - at60.x) < 0.01F);
    assert(std::abs(at30.x - at120.x) < 0.01F);
    assert(std::abs(at30.y - at60.y) < 0.01F);
    assert(std::abs(at30.y - at120.y) < 0.01F);
    assert(std::abs(at30.distance - at60.distance) < 0.01F);
    assert(std::abs(at30.distance - at120.distance) < 0.01F);

    assert(tunrun::shipDefinition(4U).speedMultiplier >
           tunrun::shipDefinition(0U).speedMultiplier);
    tunrun::FlightState rotationState;
    const tunrun::FlightInput rotationInput{
        0.0F, 0.0F, false, false, tunrun::kStarterShipId, false,
        1.0F, 0.6F, 1.0F
    };
    for (int frame = 0; frame < 30; ++frame) {
        tunrun::updateFlight(rotationState, rotationInput, 1.0F / 60.0F);
    }
    assert(rotationState.yaw > 0.0F);
    assert(rotationState.pitch > 0.0F);
    assert(rotationState.roll > 0.0F);
    assert(rotationState.x > 0.0F || rotationState.y < 0.0F);
    assert(std::isfinite(rotationState.yaw) &&
           std::isfinite(rotationState.pitch) &&
           std::isfinite(rotationState.roll));
    assert(tunrun::shipDefinition(4U).boostDrainMultiplier >
           tunrun::shipDefinition(2U).boostDrainMultiplier);
    tunrun::FlightState driftwingFlight;
    tunrun::FlightState cometFlight;
    float driftwingAccumulator = 0.0F, cometAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(driftwingFlight, tunrun::FlightInput{1.0F, 0.0F, false, false, 0U},
                              1.0F / 120.0F, driftwingAccumulator);
        tunrun::advanceFlight(cometFlight, tunrun::FlightInput{1.0F, 0.0F, false, false, 4U},
                              1.0F / 120.0F, cometAccumulator);
    }
    assert(cometFlight.x > driftwingFlight.x);
    tunrun::FlightState lowDrainFlight, highDrainFlight;
    float lowDrainAccumulator = 0.0F, highDrainAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(lowDrainFlight, tunrun::FlightInput{0.0F, 0.0F, true, false, 2U},
                              1.0F / 120.0F, lowDrainAccumulator);
        tunrun::advanceFlight(highDrainFlight, tunrun::FlightInput{0.0F, 0.0F, true, false, 4U},
                              1.0F / 120.0F, highDrainAccumulator);
    }
    assert(lowDrainFlight.boostEnergy > highDrainFlight.boostEnergy);

    // Profile serialization, strict parsing, backup recovery, and non-destructive reset.
    tunrun::Profile profile;
    profile.rootSeed = 0xD3A5B79C12345678ULL;
    profile.mouseSensitivity = 0.0065F;
    profile.totalRuns = 1U;
    profile.aetherShards = 42U;
    profile.bestScore = 98765U;
    profile.bestCombo = 27U;

    std::string profileError;
    assert(tunrun::validateProfile(profile, profileError));
    const std::string serialized = tunrun::serializeProfile(profile);
    tunrun::Profile parsedProfile;
    assert(tunrun::parseProfile(serialized, parsedProfile, profileError));
    assert(parsedProfile.rootSeed == profile.rootSeed);
    assert(parsedProfile.mouseSensitivity == profile.mouseSensitivity);
    assert(parsedProfile.aetherShards == profile.aetherShards);
    assert(parsedProfile.bestScore == profile.bestScore);
    assert(parsedProfile.bestCombo == profile.bestCombo);
    assert(parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(serialized.find("\"checksum\": \"") != std::string::npos);

    std::string tampered = serialized;
    const std::string balanceField = "\"aetherShards\": 42";
    const auto balanceOffset = tampered.find(balanceField);
    assert(balanceOffset != std::string::npos);
    tampered.replace(balanceOffset, balanceField.size(), "\"aetherShards\": 43");
    assert(!tunrun::parseProfile(tampered, parsedProfile, profileError));
    assert(profileError.find("checksum") != std::string::npos);
    tunrun::Profile invalidRecords = profile;
    invalidRecords.bestScore = tunrun::kProfileRecordCap + 1U;
    assert(!tunrun::validateProfile(invalidRecords, profileError));

    const std::string legacyV1 = R"({
      "schemaVersion": 1,
      "showFps": true,
      "reduceMotion": false,
      "mouseSteering": true,
      "fullscreen": false,
      "mouseSensitivity": 0.0065,
      "selectedShip": 0,
      "unlockedShips": [true, false, false, false, false, false, false, false],
      "aetherShards": 42,
      "singularityCores": 0,
      "totalRuns": 1,
      "totalCrashes": 0,
      "bestDistance": 12.5,
      "rootSeed": 12345,
      "runSerial": 4
    })";
    bool migratedFromLegacy = false;
    assert(tunrun::parseProfile(legacyV1, parsedProfile, profileError, &migratedFromLegacy));
    assert(migratedFromLegacy);
    assert(parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(parsedProfile.aetherShards == 42U && parsedProfile.runSerial == 4U);
    assert(parsedProfile.bestScore == 0U && parsedProfile.bestCombo == 0U);

    // v2 checksums are verified against their original field set before v3 migration.
    tunrun::Profile legacyV2Profile = profile;
    legacyV2Profile.schemaVersion = 2U;
    const std::string legacyV2 = tunrun::serializeProfile(legacyV2Profile);
    assert(legacyV2.find("bestScore") == std::string::npos);
    assert(tunrun::parseProfile(legacyV2, parsedProfile, profileError, &migratedFromLegacy));
    assert(migratedFromLegacy && parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(parsedProfile.bestScore == 0U && parsedProfile.bestCombo == 0U);
    std::string tamperedLegacyV2 = legacyV2;
    const auto legacyCurrencyField = tamperedLegacyV2.find("\"aetherShards\": 42");
    assert(legacyCurrencyField != std::string::npos);
    tamperedLegacyV2.replace(legacyCurrencyField,
                             std::string("\"aetherShards\": 42").size(),
                             "\"aetherShards\": 43");
    assert(!tunrun::parseProfile(tamperedLegacyV2, parsedProfile, profileError));
    assert(profileError.find("checksum") != std::string::npos);

    assert(!tunrun::parseProfile(std::string(tunrun::kProfileMaxBytes + 1U, 'x'),
                                 parsedProfile, profileError));

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path profileDirectory =
        std::filesystem::temp_directory_path() /
        (std::string("tunrun-profile-tests-") + std::to_string(stamp));
    std::error_code filesystemError;
    std::filesystem::remove_all(profileDirectory, filesystemError);
    // A v1 primary upgrades to v3 while the exact legacy payload remains in backup.
    const std::filesystem::path migrationDirectory =
        std::filesystem::temp_directory_path() /
        (std::string("tunrun-profile-migration-tests-") + std::to_string(stamp));
    std::filesystem::remove_all(migrationDirectory, filesystemError);
    std::filesystem::create_directories(migrationDirectory, filesystemError);
    assert(!filesystemError);
    assert(writeTestFile(migrationDirectory / "profile.json", legacyV1));
    tunrun::ProfileStore migrationStore(migrationDirectory);
    const auto migratedLoad = migrationStore.load();
    assert(migratedLoad.status == tunrun::ProfileLoadStatus::Loaded);
    assert(migratedLoad.profile.schemaVersion == tunrun::kProfileSchemaVersion);
    std::ifstream migratedFile(migrationDirectory / "profile.json", std::ios::binary);
    const std::string migratedText{std::istreambuf_iterator<char>(migratedFile),
                                   std::istreambuf_iterator<char>()};
    assert(migratedText.find("\"schemaVersion\": 3") != std::string::npos);
    assert(migratedText.find("\"checksum\": \"") != std::string::npos);
    std::ifstream legacyBackupFile(migrationDirectory / "profile.bak", std::ios::binary);
    const std::string legacyBackup{std::istreambuf_iterator<char>(legacyBackupFile),
                                   std::istreambuf_iterator<char>()};
    assert(legacyBackup.find("\"schemaVersion\": 1") != std::string::npos);
    assert(tunrun::parseProfile(legacyBackup, parsedProfile, profileError, &migratedFromLegacy));
    assert(migratedFromLegacy && parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    std::filesystem::remove_all(migrationDirectory, filesystemError);

    // Exercise a real v2-on-disk upgrade and verify its backup remains intact.
    const std::filesystem::path legacyV2Directory =
        std::filesystem::temp_directory_path() /
        (std::string("tunrun-profile-v2-migration-tests-") + std::to_string(stamp));
    std::filesystem::remove_all(legacyV2Directory, filesystemError);
    std::filesystem::create_directories(legacyV2Directory, filesystemError);
    assert(!filesystemError);
    assert(writeTestFile(legacyV2Directory / "profile.json", legacyV2));
    tunrun::ProfileStore v2MigrationStore(legacyV2Directory);
    const auto migratedV2Load = v2MigrationStore.load();
    assert(migratedV2Load.status == tunrun::ProfileLoadStatus::Loaded);
    assert(migratedV2Load.profile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(migratedV2Load.profile.bestScore == 0U && migratedV2Load.profile.bestCombo == 0U);
    std::ifstream legacyV2BackupFile(legacyV2Directory / "profile.bak", std::ios::binary);
    const std::string legacyV2Backup{std::istreambuf_iterator<char>(legacyV2BackupFile),
                                     std::istreambuf_iterator<char>()};
    assert(legacyV2Backup.find("\"schemaVersion\": 2") != std::string::npos);
    std::filesystem::remove_all(legacyV2Directory, filesystemError);

    tunrun::ProfileStore store(profileDirectory);
    assert(store.load().status == tunrun::ProfileLoadStatus::NotFound);
    assert(store.save(profile).success);

    auto firstLoad = store.load();
    assert(firstLoad.status == tunrun::ProfileLoadStatus::Loaded);
    assert(firstLoad.profile.rootSeed == profile.rootSeed);
    assert(firstLoad.profile.totalRuns == 1U);

    tunrun::Profile nextProfile = profile;
    nextProfile.runSerial = 7U;
    nextProfile.totalRuns = 2U;
    nextProfile.aetherShards = 64U;
    assert(store.save(nextProfile).success); // preserves the previous primary as backup

    assert(writeTestFile(profileDirectory / "profile.json",
                         "{ this is intentionally corrupted"));
    const auto recovered = store.load();
    assert(recovered.status == tunrun::ProfileLoadStatus::RecoveredBackup);
    assert(recovered.profile.runSerial == profile.runSerial);
    assert(recovered.profile.aetherShards == profile.aetherShards);
    assert(store.save(recovered.profile, true).success); // repair without overwriting the good backup
    assert(store.load().status == tunrun::ProfileLoadStatus::Loaded);

    for (const char* name : {"profile.json", "profile.bak"}) {
        assert(writeTestFile(profileDirectory / name, "{ damaged"));
    }
    assert(store.load().status == tunrun::ProfileLoadStatus::RecoveryRequired);
    tunrun::Profile defaults;
    defaults.rootSeed = 0x8877665544332211ULL;
    assert(store.resetToDefaults(defaults).success);
    const auto afterReset = store.load();
    assert(afterReset.status == tunrun::ProfileLoadStatus::Loaded);
    assert(afterReset.profile.rootSeed == defaults.rootSeed);
    tunrun::Profile economyProfile;
    economyProfile.rootSeed = 99U;
    economyProfile.aetherShards = 10000U;
    const auto purchase = tunrun::purchaseShip(economyProfile, 1U);
    assert(purchase == tunrun::ShipTransactionStatus::Purchased);
    assert(economyProfile.aetherShards == 2000U && economyProfile.unlockedShips[1U]);
    assert(tunrun::purchaseShip(economyProfile, 1U) ==
           tunrun::ShipTransactionStatus::AlreadyUnlocked);
    assert(economyProfile.aetherShards == 2000U); // duplicate activation cannot charge twice
    assert(tunrun::equipShip(economyProfile, 1U) == tunrun::ShipTransactionStatus::Equipped);
    assert(economyProfile.selectedShip == 1U);
    const auto shardsBeforeFailedBuy = economyProfile.aetherShards;
    assert(tunrun::purchaseShip(economyProfile, 2U) ==
           tunrun::ShipTransactionStatus::InsufficientAetherShards);
    assert(economyProfile.aetherShards == shardsBeforeFailedBuy);
    assert(!economyProfile.unlockedShips[2U]);
    assert(tunrun::equipShip(economyProfile, 2U) == tunrun::ShipTransactionStatus::ShipLocked);
    economyProfile.singularityCores = 200U;
    assert(tunrun::purchaseShip(economyProfile, 5U) == tunrun::ShipTransactionStatus::Purchased);
    assert(economyProfile.singularityCores == 0U && economyProfile.unlockedShips[5U]);
    assert(tunrun::purchaseShip(economyProfile, 6U) ==
           tunrun::ShipTransactionStatus::InsufficientSingularityCores);
    assert(tunrun::purchaseShip(economyProfile, 8U) ==
           tunrun::ShipTransactionStatus::InvalidShip);

    bool foundPreservedDamagedFile = false;
    for (const auto& entry : std::filesystem::directory_iterator(profileDirectory)) {
        if (entry.path().filename().string().find(".corrupt-") != std::string::npos) {
            foundPreservedDamagedFile = true;
            break;
        }
    }
    assert(foundPreservedDamagedFile);
    std::filesystem::remove_all(profileDirectory, filesystemError);

    std::cout << "TUNRUN input, physics, course, and profile-persistence tests passed.\n";
}
