#include "app/input_state.hpp"
#include "app/flight_physics.hpp"
#include "app/economy.hpp"
#include "app/raw_mouse.hpp"
#include "app/save_profile.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    stack.push(Screen::Preview);
    stack.push(Screen::Pause);
    stack.push(Screen::Settings);
    assert(stack.current() == Screen::Settings);
    assert(stack.pop() && stack.current() == Screen::Pause);
    assert(stack.pop() && stack.current() == Screen::Preview);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.push(Screen::Settings);
    stack.replace(Screen::Credits);
    assert(stack.current() == Screen::Credits);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.reset();
    assert(stack.current() == Screen::MainMenu && stack.size() == 1U);

    float mouseX = 0.0F;
    float mouseY = 0.0F;
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{100.0F, -50.0F}, 0.004F);
    assert(std::abs(mouseX - 0.4F) < 0.0001F);
    assert(std::abs(mouseY - 0.2F) < 0.0001F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{5000.0F, -5000.0F}, 0.004F);
    assert(mouseX == 3.1F && mouseY == 3.1F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{1.0F, 1.0F}, -1.0F);
    assert(mouseX == 3.1F && mouseY == 3.1F);

    constexpr std::uint64_t seed = 0x123456789ABCDEF0ULL;
    const auto section = tunrun::sampleCourse(seed, 1.25);
    assert(section.radius >= tunrun::kCourseMinRadius && section.radius <= tunrun::kCourseMaxRadius);
    const tunrun::TunnelCrossSection centredSection{0.0F, 0.0F, section.radius, section.twist};
    assert(!tunrun::collidesWithTunnelWall(0.0F, 0.0F, centredSection));
    const float safeRadius = section.radius - tunrun::kCraftCollisionRadius;
    assert(tunrun::collidesWithTunnelWall(safeRadius + 0.01F, 0.0F, centredSection));

    assert(tunrun::courseHash(seed) == tunrun::courseHash(seed));
    assert(tunrun::courseHash(seed) != tunrun::courseHash(seed + 1U));
    const auto gate = tunrun::gateAt(seed, 3U);
    assert(!tunrun::collidesWithGate(gate.offsetX, gate.offsetY, gate));
    assert(tunrun::collidesWithGate(gate.offsetX + gate.apertureRadius, gate.offsetY, gate));
    assert(tunrun::crossesGatePlane(gate.distance - 0.1, gate.distance + 0.1, gate));
    assert(!tunrun::crossesGatePlane(gate.distance + 0.5, gate.distance + 0.7, gate));

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

    std::string profileError;
    assert(tunrun::validateProfile(profile, profileError));
    const std::string serialized = tunrun::serializeProfile(profile);
    tunrun::Profile parsedProfile;
    assert(tunrun::parseProfile(serialized, parsedProfile, profileError));
    assert(parsedProfile.rootSeed == profile.rootSeed);
    assert(parsedProfile.mouseSensitivity == profile.mouseSensitivity);
    assert(parsedProfile.aetherShards == profile.aetherShards);
    assert(parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(serialized.find("\"checksum\": \"") != std::string::npos);

    std::string tampered = serialized;
    const std::string balanceField = "\"aetherShards\": 42";
    const auto balanceOffset = tampered.find(balanceField);
    assert(balanceOffset != std::string::npos);
    tampered.replace(balanceOffset, balanceField.size(), "\"aetherShards\": 43");
    assert(!tunrun::parseProfile(tampered, parsedProfile, profileError));
    assert(profileError.find("checksum") != std::string::npos);

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
    bool migratedFromV1 = false;
    assert(tunrun::parseProfile(legacyV1, parsedProfile, profileError, &migratedFromV1));
    assert(migratedFromV1);
    assert(parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    assert(parsedProfile.aetherShards == 42U && parsedProfile.runSerial == 4U);

    assert(!tunrun::parseProfile(std::string(tunrun::kProfileMaxBytes + 1U, 'x'),
                                 parsedProfile, profileError));

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path profileDirectory =
        std::filesystem::temp_directory_path() /
        (std::string("tunrun-profile-tests-") + std::to_string(stamp));
    std::error_code filesystemError;
    std::filesystem::remove_all(profileDirectory, filesystemError);
    // A v1 primary upgrades to v2 while the exact legacy payload remains in backup.
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
    assert(migratedText.find("\"schemaVersion\": 2") != std::string::npos);
    assert(migratedText.find("\"checksum\": \"") != std::string::npos);
    std::ifstream legacyBackupFile(migrationDirectory / "profile.bak", std::ios::binary);
    const std::string legacyBackup{std::istreambuf_iterator<char>(legacyBackupFile),
                                   std::istreambuf_iterator<char>()};
    assert(legacyBackup.find("\"schemaVersion\": 1") != std::string::npos);
    assert(tunrun::parseProfile(legacyBackup, parsedProfile, profileError, &migratedFromV1));
    assert(migratedFromV1 && parsedProfile.schemaVersion == tunrun::kProfileSchemaVersion);
    std::filesystem::remove_all(migrationDirectory, filesystemError);

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
    economyProfile.aetherShards = 100U;
    const auto purchase = tunrun::purchaseShip(economyProfile, 1U);
    assert(purchase == tunrun::ShipTransactionStatus::Purchased);
    assert(economyProfile.aetherShards == 20U && economyProfile.unlockedShips[1U]);
    assert(tunrun::purchaseShip(economyProfile, 1U) ==
           tunrun::ShipTransactionStatus::AlreadyUnlocked);
    assert(economyProfile.aetherShards == 20U); // duplicate activation cannot charge twice
    assert(tunrun::equipShip(economyProfile, 1U) == tunrun::ShipTransactionStatus::Equipped);
    assert(economyProfile.selectedShip == 1U);
    const auto shardsBeforeFailedBuy = economyProfile.aetherShards;
    assert(tunrun::purchaseShip(economyProfile, 2U) ==
           tunrun::ShipTransactionStatus::InsufficientAetherShards);
    assert(economyProfile.aetherShards == shardsBeforeFailedBuy);
    assert(!economyProfile.unlockedShips[2U]);
    assert(tunrun::equipShip(economyProfile, 2U) == tunrun::ShipTransactionStatus::ShipLocked);
    economyProfile.singularityCores = 2U;
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
