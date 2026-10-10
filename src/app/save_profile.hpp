#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace tunrun {

inline constexpr std::uint32_t kProfileSchemaVersion = 3U;
inline constexpr std::size_t kProfileMaxBytes = 65536U;
inline constexpr std::size_t kProfileShipCount = 8U;
inline constexpr std::uint64_t kProfileRecordCap = 9000000000000000ULL;

struct Profile {
    std::uint32_t schemaVersion = kProfileSchemaVersion;
    bool showFps = true;
    bool reduceMotion = false;
    bool mouseSteering = true;
    bool fullscreen = true;
    float mouseSensitivity = 0.004F;
    std::uint32_t selectedShip = 0U;
    std::array<bool, kProfileShipCount> unlockedShips{
        true, false, false, false, false, false, false, false
    };
    std::uint64_t aetherShards = 0U;
    std::uint64_t singularityCores = 0U;
    std::uint64_t totalRuns = 0U;
    std::uint64_t totalCrashes = 0U;
    double bestDistance = 0.0;
    std::uint64_t bestScore = 0U;
    std::uint64_t bestCombo = 0U;
    std::uint64_t rootSeed = 0U;
    std::uint64_t runSerial = 0U;
};

enum class ProfileLoadStatus {
    Loaded,
    RecoveredBackup,
    NotFound,
    RecoveryRequired,
    Error
};

struct ProfileLoadResult {
    Profile profile{};
    ProfileLoadStatus status = ProfileLoadStatus::Error;
    std::string message;
};

struct ProfileSaveResult {
    bool success = false;
    std::string message;
};

[[nodiscard]] std::string serializeProfile(const Profile& profile);
// Schemas v1 and v2 remain readable and are migrated in memory. The optional
// flag is true whenever an older document parses and validates successfully.
[[nodiscard]] bool parseProfile(std::string_view json, Profile& output,
                                std::string& error,
                                bool* migratedFromLegacyVersion = nullptr);
[[nodiscard]] bool validateProfile(const Profile& profile,
                                   std::string& error);

class ProfileStore {
public:
    explicit ProfileStore(std::filesystem::path directory = {});
    [[nodiscard]] static ProfileStore forCurrentUser();
    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] ProfileLoadResult load() const noexcept;
    // preserveBackup is used only when restoring a known-good backup; it
    // prevents a corrupt primary from overwriting that known-good copy.
    [[nodiscard]] ProfileSaveResult save(const Profile& profile,
                                         bool preserveBackup = false) const noexcept;
    // Explicit user-directed recovery: damaged files are renamed, not deleted.
    [[nodiscard]] ProfileSaveResult resetToDefaults(const Profile& profile) const noexcept;

private:
    std::filesystem::path directory_;
};

} // namespace tunrun
