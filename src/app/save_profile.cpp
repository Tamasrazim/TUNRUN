#include "app/save_profile.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace tunrun {
namespace {
enum class JsonKind { Number, Boolean, String, BooleanArray };
struct JsonValue {
    JsonKind kind = JsonKind::String;
    std::string text;
    bool boolean = false;
    std::vector<bool> booleans;
};

class FlatJsonParser {
public:
    explicit FlatJsonParser(std::string_view input) : input_(input) {}

    bool parse(std::unordered_map<std::string, JsonValue>& fields,
               std::string& error) {
        skipWhitespace();
        if (!consume('{')) return fail(error, "profile must be a JSON object");
        skipWhitespace();
        if (consume('}')) return finish(error);
        for (;;) {
            std::string key;
            if (!parseString(key, error)) return false;
            skipWhitespace();
            if (!consume(':')) return fail(error, "expected ':' after JSON key");
            skipWhitespace();
            JsonValue value;
            if (!parseValue(value, error)) return false;
            if (!fields.emplace(key, std::move(value)).second) {
                return fail(error, "duplicate JSON key");
            }
            skipWhitespace();
            if (consume('}')) return finish(error);
            if (!consume(',')) return fail(error, "expected ',' or '}' in JSON object");
            skipWhitespace();
        }
    }

private:
    bool finish(std::string& error) {
        skipWhitespace();
        return position_ == input_.size() || fail(error, "trailing data after JSON object");
    }
    bool fail(std::string& error, const char* message) const {
        error = message;
        return false;
    }
    void skipWhitespace() {
        while (position_ < input_.size()) {
            const char c = input_[position_];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
            ++position_;
        }
    }
    bool consume(char expected) {
        if (position_ >= input_.size() || input_[position_] != expected) return false;
        ++position_;
        return true;
    }
    bool parseString(std::string& output, std::string& error) {
        if (!consume('"')) return fail(error, "expected JSON string");
        output.clear();
        while (position_ < input_.size()) {
            const unsigned char c = static_cast<unsigned char>(input_[position_++]);
            if (c == '"') return true;
            if (c < 0x20U) return fail(error, "control character in JSON string");
            if (c != '\\') {
                output.push_back(static_cast<char>(c));
                continue;
            }
            if (position_ >= input_.size()) return fail(error, "truncated JSON escape");
            const char escaped = input_[position_++];
            switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: return fail(error, "unsupported JSON string escape");
            }
        }
        return fail(error, "unterminated JSON string");
    }
    bool parseValue(JsonValue& value, std::string& error) {
        if (position_ >= input_.size()) return fail(error, "missing JSON value");
        if (input_[position_] == '"') {
            value.kind = JsonKind::String;
            return parseString(value.text, error);
        }
        if (input_[position_] == 't' && input_.substr(position_, 4) == "true") {
            position_ += 4;
            value.kind = JsonKind::Boolean;
            value.boolean = true;
            return true;
        }
        if (input_[position_] == 'f' && input_.substr(position_, 5) == "false") {
            position_ += 5;
            value.kind = JsonKind::Boolean;
            value.boolean = false;
            return true;
        }
        if (input_[position_] == '[') {
            ++position_;
            value.kind = JsonKind::BooleanArray;
            value.booleans.clear();
            skipWhitespace();
            if (consume(']')) return true;
            for (;;) {
                skipWhitespace();
                if (input_.substr(position_, 4) == "true") {
                    value.booleans.push_back(true);
                    position_ += 4;
                } else if (input_.substr(position_, 5) == "false") {
                    value.booleans.push_back(false);
                    position_ += 5;
                } else {
                    return fail(error, "unlockedShips must contain only booleans");
                }
                if (value.booleans.size() > kProfileShipCount) {
                    return fail(error, "too many unlocked ship flags");
                }
                skipWhitespace();
                if (consume(']')) return true;
                if (!consume(',')) return fail(error, "expected ',' or ']' in array");
            }
        }

        const std::size_t start = position_;
        if (input_[position_] == '-') ++position_;
        if (position_ >= input_.size()) return fail(error, "invalid JSON number");
        if (input_[position_] == '0') {
            ++position_;
            if (position_ < input_.size() && input_[position_] >= '0' &&
                input_[position_] <= '9') return fail(error, "leading zero in JSON number");
        } else {
            if (input_[position_] < '1' || input_[position_] > '9')
                return fail(error, "invalid JSON number");
            while (position_ < input_.size() && input_[position_] >= '0' &&
                   input_[position_] <= '9') ++position_;
        }
        if (position_ < input_.size() && input_[position_] == '.') {
            ++position_;
            const std::size_t fractionStart = position_;
            while (position_ < input_.size() && input_[position_] >= '0' &&
                   input_[position_] <= '9') ++position_;
            if (fractionStart == position_) return fail(error, "invalid JSON fraction");
        }
        if (position_ < input_.size() &&
            (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() &&
                (input_[position_] == '+' || input_[position_] == '-')) ++position_;
            const std::size_t exponentStart = position_;
            while (position_ < input_.size() && input_[position_] >= '0' &&
                   input_[position_] <= '9') ++position_;
            if (exponentStart == position_) return fail(error, "invalid JSON exponent");
        }
        value.kind = JsonKind::Number;
        value.text.assign(input_.substr(start, position_ - start));
        return true;
    }

    std::string_view input_;
    std::size_t position_ = 0U;
};

bool readProfileFile(const std::filesystem::path& path, std::string& content,
                     bool& exists, std::string& error) {
    std::error_code ec;
    exists = std::filesystem::exists(path, ec);
    if (ec) {
        error = "unable to inspect profile file";
        return false;
    }
    if (!exists) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > kProfileMaxBytes) {
        error = "profile file is oversized or unreadable";
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "unable to open profile file";
        return false;
    }
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (!file.good() && !file.eof()) {
        error = "unable to read profile file";
        return false;
    }
    if (content.size() > kProfileMaxBytes) {
        error = "profile file is oversized";
        return false;
    }
    return true;
}

const JsonValue* findValue(const std::unordered_map<std::string, JsonValue>& values,
                           const char* key, JsonKind kind, std::string& error) {
    const auto it = values.find(key);
    if (it == values.end()) {
        error = std::string("missing profile field: ") + key;
        return nullptr;
    }
    if (it->second.kind != kind) {
        error = std::string("wrong type for profile field: ") + key;
        return nullptr;
    }
    return &it->second;
}

template <typename UInt>
bool parseUnsignedField(const std::unordered_map<std::string, JsonValue>& values,
                        const char* key, UInt& output, std::string& error) {
    const JsonValue* value = findValue(values, key, JsonKind::Number, error);
    if (!value) return false;
    if (value->text.empty() || value->text.front() == '-') {
        error = std::string("negative or empty unsigned field: ") + key;
        return false;
    }
    UInt parsed{};
    const auto result = std::from_chars(value->text.data(),
        value->text.data() + value->text.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != value->text.data() + value->text.size()) {
        error = std::string("invalid integer field: ") + key;
        return false;
    }
    output = parsed;
    return true;
}
bool parseBooleanField(const std::unordered_map<std::string, JsonValue>& values,
                       const char* key, bool& output, std::string& error) {
    const JsonValue* value = findValue(values, key, JsonKind::Boolean, error);
    if (!value) return false;
    output = value->boolean;
    return true;
}
bool parseFloatField(const std::unordered_map<std::string, JsonValue>& values,
                     const char* key, float& output, std::string& error) {
    const JsonValue* value = findValue(values, key, JsonKind::Number, error);
    if (!value) return false;
    char* end = nullptr;
    const float parsed = std::strtof(value->text.c_str(), &end);
    if (end != value->text.c_str() + value->text.size() || !std::isfinite(parsed)) {
        error = std::string("invalid finite number field: ") + key;
        return false;
    }
    output = parsed;
    return true;
}
bool parseDoubleField(const std::unordered_map<std::string, JsonValue>& values,
                      const char* key, double& output, std::string& error) {
    const JsonValue* value = findValue(values, key, JsonKind::Number, error);
    if (!value) return false;
    char* end = nullptr;
    const double parsed = std::strtod(value->text.c_str(), &end);
    if (end != value->text.c_str() + value->text.size() || !std::isfinite(parsed)) {
        error = std::string("invalid finite number field: ") + key;
        return false;
    }
    output = parsed;
    return true;
}

bool hidePath(const std::filesystem::path& path) noexcept {
#if defined(_WIN32)
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return false;
    return (attributes & FILE_ATTRIBUTE_HIDDEN) != 0U ||
           SetFileAttributesW(path.c_str(), attributes | FILE_ATTRIBUTE_HIDDEN) != 0;
#else
    (void)path;
    return true;
#endif
}

bool writeAndFlush(const std::filesystem::path& path, std::string_view content,
                   std::string& error) {
#if defined(_WIN32)
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = "unable to create temporary profile";
        return false;
    }
    std::size_t offset = 0U;
    bool ok = true;
    while (offset < content.size()) {
        const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(
            content.size() - offset, static_cast<std::size_t>(1U << 20U)));
        DWORD written = 0U;
        if (!WriteFile(file, content.data() + offset, chunk, &written, nullptr) ||
            written == 0U) {
            ok = false;
            error = "unable to write temporary profile";
            break;
        }
        offset += written;
    }
    if (ok && !FlushFileBuffers(file)) {
        ok = false;
        error = "unable to flush temporary profile";
    }
    CloseHandle(file);
    return ok;
#else
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "unable to create temporary profile";
        return false;
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    if (!file) {
        error = "unable to write temporary profile";
        return false;
    }
    file.close();
    return true;
#endif
}

bool atomicReplace(const std::filesystem::path& source,
                   const std::filesystem::path& destination,
                   std::string& error) {
#if defined(_WIN32)
    if (!MoveFileExW(source.c_str(), destination.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "atomic profile replacement failed";
        return false;
    }
    return true;
#else
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    if (ec) {
        error = "atomic profile replacement failed";
        return false;
    }
    return true;
#endif
}

bool copyFile(const std::filesystem::path& source,
              const std::filesystem::path& destination,
              std::string& error) {
#if defined(_WIN32)
    if (!CopyFileW(source.c_str(), destination.c_str(), FALSE)) {
        error = "unable to preserve profile backup";
        return false;
    }
    return true;
#else
    std::error_code ec;
    std::filesystem::copy_file(source, destination,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        error = "unable to preserve profile backup";
        return false;
    }
    return true;
#endif
}

} // namespace

bool validateProfile(const Profile& profile, std::string& error) {
    if (profile.schemaVersion != kProfileSchemaVersion) {
        error = "unsupported profile schema version";
        return false;
    }
    if (profile.selectedShip >= kProfileShipCount) {
        error = "selected ship id is out of range";
        return false;
    }
    if (!profile.unlockedShips[0]) {
        error = "starter ship must remain unlocked";
        return false;
    }
    if (!std::isfinite(profile.mouseSensitivity) ||
        profile.mouseSensitivity < 0.0001F || profile.mouseSensitivity > 0.05F) {
        error = "mouse sensitivity is outside its legal range";
        return false;
    }
    if (!std::isfinite(profile.bestDistance) || profile.bestDistance < 0.0 ||
        profile.bestDistance > 1.0e12) {
        error = "best distance is outside its legal range";
        return false;
    }
    if (profile.bestScore > kProfileRecordCap || profile.bestCombo > kProfileRecordCap) {
        error = "best score or combo exceeds the supported record cap";
        return false;
    }
    if (profile.totalCrashes > profile.totalRuns) {
        error = "crash count exceeds recorded runs";
        return false;
    }
    error.clear();
    return true;
}

namespace {
std::string serializeProfilePayload(const Profile& profile) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "{\n"
        << "  \"schemaVersion\": " << profile.schemaVersion << ",\n"
        << "  \"showFps\": " << (profile.showFps ? "true" : "false") << ",\n"
        << "  \"reduceMotion\": " << (profile.reduceMotion ? "true" : "false") << ",\n"
        << "  \"mouseSteering\": " << (profile.mouseSteering ? "true" : "false") << ",\n"
        << "  \"fullscreen\": " << (profile.fullscreen ? "true" : "false") << ",\n"
        << "  \"mouseSensitivity\": " << std::setprecision(9) << profile.mouseSensitivity << ",\n"
        << "  \"selectedShip\": " << profile.selectedShip << ",\n"
        << "  \"unlockedShips\": [";
    for (std::size_t i = 0; i < profile.unlockedShips.size(); ++i) {
        if (i != 0U) out << ", ";
        out << (profile.unlockedShips[i] ? "true" : "false");
    }
    out << "],\n"
        << "  \"aetherShards\": " << profile.aetherShards << ",\n"
        << "  \"singularityCores\": " << profile.singularityCores << ",\n"
        << "  \"totalRuns\": " << profile.totalRuns << ",\n"
        << "  \"totalCrashes\": " << profile.totalCrashes << ",\n"
        << "  \"bestDistance\": " << std::setprecision(17) << profile.bestDistance << ",\n";
    // Omitting v3-only fields for older schema versions preserves the exact
    // canonical payload used to verify legacy checksums during migration.
    if (profile.schemaVersion >= 3U) {
        out << "  \"bestScore\": " << profile.bestScore << ",\n"
            << "  \"bestCombo\": " << profile.bestCombo << ",\n";
    }
    out << "  \"rootSeed\": " << profile.rootSeed << ",\n"
        << "  \"runSerial\": " << profile.runSerial << "\n"
        << "}\n";
    return out.str();
}

// FNV-1a detects accidental value corruption only; it is not authentication,
// encryption, or a defence against a player editing their local profile.
std::string profileChecksumHex(std::string_view content) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : content) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}
} // namespace

std::string serializeProfile(const Profile& profile) {
    const std::string payload = serializeProfilePayload(profile);
    const std::size_t closing = payload.rfind("}\n");
    if (closing == std::string::npos) return payload;
    std::string output = payload.substr(0U, closing);
    if (!output.empty() && output.back() == '\n') output.pop_back();
    output += ",\n  \"checksum\": \"" + profileChecksumHex(payload) + "\"\n}\n";
    return output;
}

bool parseProfile(std::string_view json, Profile& output, std::string& error,
                  bool* migratedFromLegacyVersion) {
    if (migratedFromLegacyVersion) *migratedFromLegacyVersion = false;
    if (json.empty() || json.size() > kProfileMaxBytes) {
        error = "profile is empty or exceeds the size limit";
        return false;
    }
    std::unordered_map<std::string, JsonValue> values;
    FlatJsonParser parser(json);
    if (!parser.parse(values, error)) return false;

    std::uint32_t sourceVersion = 0U;
    if (!parseUnsignedField(values, "schemaVersion", sourceVersion, error)) return false;
    if (sourceVersion < 1U || sourceVersion > kProfileSchemaVersion) {
        error = "unsupported profile schema version";
        return false;
    }
    const bool migrateLegacy = sourceVersion < kProfileSchemaVersion;
    const std::size_t requiredFields = sourceVersion == 1U ? 15U
        : sourceVersion == 2U ? 16U : 18U;
    if (values.size() != requiredFields) {
        error = "profile has missing or unexpected fields";
        return false;
    }

    Profile parsed{};
    // Keep the source version until its canonical checksum has been verified.
    parsed.schemaVersion = sourceVersion;
    if (!parseBooleanField(values, "showFps", parsed.showFps, error) ||
        !parseBooleanField(values, "reduceMotion", parsed.reduceMotion, error) ||
        !parseBooleanField(values, "mouseSteering", parsed.mouseSteering, error) ||
        !parseBooleanField(values, "fullscreen", parsed.fullscreen, error) ||
        !parseFloatField(values, "mouseSensitivity", parsed.mouseSensitivity, error) ||
        !parseUnsignedField(values, "selectedShip", parsed.selectedShip, error) ||
        !parseUnsignedField(values, "aetherShards", parsed.aetherShards, error) ||
        !parseUnsignedField(values, "singularityCores", parsed.singularityCores, error) ||
        !parseUnsignedField(values, "totalRuns", parsed.totalRuns, error) ||
        !parseUnsignedField(values, "totalCrashes", parsed.totalCrashes, error) ||
        !parseDoubleField(values, "bestDistance", parsed.bestDistance, error)) return false;
    if (sourceVersion >= 3U &&
        (!parseUnsignedField(values, "bestScore", parsed.bestScore, error) ||
         !parseUnsignedField(values, "bestCombo", parsed.bestCombo, error))) return false;
    if (!parseUnsignedField(values, "rootSeed", parsed.rootSeed, error) ||
        !parseUnsignedField(values, "runSerial", parsed.runSerial, error)) return false;

    const JsonValue* ships = findValue(values, "unlockedShips", JsonKind::BooleanArray, error);
    if (!ships) return false;
    if (ships->booleans.size() != kProfileShipCount) {
        error = "unlockedShips must contain exactly eight entries";
        return false;
    }
    for (std::size_t i = 0; i < kProfileShipCount; ++i) {
        parsed.unlockedShips[i] = ships->booleans[i];
    }
    if (sourceVersion >= 2U) {
        const JsonValue* checksum = findValue(values, "checksum", JsonKind::String, error);
        if (!checksum) return false;
        const std::string expected = profileChecksumHex(serializeProfilePayload(parsed));
        if (checksum->text.size() != 16U || checksum->text != expected) {
            error = "profile checksum mismatch";
            return false;
        }
    }

    // Verify and parse using the original schema before validating under v3 rules.
    parsed.schemaVersion = kProfileSchemaVersion;
    if (!validateProfile(parsed, error)) return false;

    output = parsed;
    if (migratedFromLegacyVersion) *migratedFromLegacyVersion = migrateLegacy;
    error.clear();
    return true;
}

ProfileStore::ProfileStore(std::filesystem::path directory)
    : directory_(std::move(directory)) {}

ProfileStore ProfileStore::forCurrentUser() {
#if defined(_WIN32)
    const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0U);
    if (required == 0U) return ProfileStore{};
    std::vector<wchar_t> buffer(static_cast<std::size_t>(required));
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0U || length >= buffer.size()) return ProfileStore{};
    return ProfileStore(std::filesystem::path(std::wstring(buffer.data(), length)) /
                        L"Tamasrazim" / L"TUNRUN");
#else
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData == nullptr || *localAppData == '\0') return ProfileStore{};
    return ProfileStore(std::filesystem::path(localAppData) / "Tamasrazim" / "TUNRUN");
#endif
}

const std::filesystem::path& ProfileStore::directory() const noexcept {
    return directory_;
}

ProfileLoadResult ProfileStore::load() const noexcept {
    ProfileLoadResult result;
    if (directory_.empty()) {
        result.status = ProfileLoadStatus::Error;
        result.message = "local profile directory could not be resolved";
        return result;
    }
    try {
        const auto primary = directory_ / "profile.json";
        const auto backup = directory_ / "profile.bak";
        std::string primaryText, backupText, readError, parseError;
        bool primaryExists = false, backupExists = false;
        bool migrated = false;
        if (readProfileFile(primary, primaryText, primaryExists, readError) && primaryExists) {
            if (parseProfile(primaryText, result.profile, parseError, &migrated)) {
                result.status = ProfileLoadStatus::Loaded;
                if (migrated) {
                    const auto migration = save(result.profile, false);
                    result.message = migration.success
                        ? "profile migrated to schema v3; previous profile retained as backup"
                        : "profile loaded; schema migration will retry on the next save";
                } else {
                    result.message = "profile loaded";
                }
                return result;
            }
        }
        std::error_code existsError;
        const bool primaryWasPresent = primaryExists ||
            std::filesystem::exists(primary, existsError);
        readError.clear();
        migrated = false;
        if (readProfileFile(backup, backupText, backupExists, readError) && backupExists) {
            if (parseProfile(backupText, result.profile, parseError, &migrated)) {
                result.status = ProfileLoadStatus::RecoveredBackup;
                if (migrated) {
                    const auto migration = save(result.profile, true);
                    result.message = migration.success
                        ? "recovered and migrated profile from backup; legacy copy preserved"
                        : "recovered older profile from backup; schema migration will retry on the next save";
                } else {
                    result.message = "recovered profile from backup";
                }
                return result;
            }
        }
        existsError.clear();
        const bool backupWasPresent = backupExists ||
            std::filesystem::exists(backup, existsError);
        if (!primaryWasPresent && !backupWasPresent) {
            result.status = ProfileLoadStatus::NotFound;
            result.message = "no existing profile";
        } else {
            result.status = ProfileLoadStatus::RecoveryRequired;
            result.message = "primary and backup profile are unavailable or invalid; damaged files were preserved";
        }
        return result;
    } catch (...) {
        result.status = ProfileLoadStatus::Error;
        result.message = "unexpected error while loading profile";
        return result;
    }
}

ProfileSaveResult ProfileStore::save(const Profile& profile,
                                     bool preserveBackup) const noexcept {
    std::string error;
    if (!validateProfile(profile, error)) return {false, error};
    if (directory_.empty()) return {false, "local profile directory could not be resolved"};
    try {
        std::error_code ec;
        std::filesystem::create_directories(directory_, ec);
        if (ec) return {false, "unable to create profile directory"};
        const auto primary = directory_ / "profile.json";
        const auto backup = directory_ / "profile.bak";
        const auto temporary = directory_ / "profile.json.tmp";
        const std::string content = serializeProfile(profile);
        if (content.size() > kProfileMaxBytes) return {false, "serialized profile exceeds size limit"};
        if (!writeAndFlush(temporary, content, error)) {
            std::filesystem::remove(temporary, ec);
            return {false, error};
        }
        const bool primaryExists = std::filesystem::exists(primary, ec) && !ec;
        if (primaryExists && !preserveBackup) {
            if (!copyFile(primary, backup, error)) {
                std::filesystem::remove(temporary, ec);
                return {false, error};
            }
            (void)hidePath(backup);
        }
        if (!atomicReplace(temporary, primary, error)) {
            std::filesystem::remove(temporary, ec);
            return {false, error};
        }
        (void)hidePath(primary);
        (void)hidePath(directory_);
        return {true, "profile saved"};
    } catch (...) {
        return {false, "unexpected error while saving profile"};
    }
}

ProfileSaveResult ProfileStore::resetToDefaults(const Profile& profile) const noexcept {
    if (directory_.empty()) return {false, "local profile directory could not be resolved"};
    try {
        std::error_code ec;
        std::filesystem::create_directories(directory_, ec);
        if (ec) return {false, "unable to create profile directory"};
        const auto stamp = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        for (const char* name : {"profile.json", "profile.bak", "profile.json.tmp"}) {
            const auto source = directory_ / name;
            if (!std::filesystem::exists(source, ec) || ec) {
                ec.clear();
                continue;
            }
            const auto target = directory_ / (std::string(name) + ".corrupt-" + stamp);
            std::filesystem::rename(source, target, ec);
            if (ec) return {false, "unable to preserve damaged profile before reset"};
        }
        return save(profile, true);
    } catch (...) {
        return {false, "unexpected error while resetting profile"};
    }
}

} // namespace tunrun
