#pragma once
#include "app/procedural_course.hpp"
#include <cstdint>
#include <string>
#include <string_view>
namespace tunrun {
struct SeedTextParseResult {
    bool valid = false;
    std::uint64_t seed = 0U;
    bool hexadecimal = false;
    std::string normalizedText;
    std::string error;
};
inline bool parseHexSeed(std::string_view text, std::uint64_t& value) noexcept {
    if (text.empty() || text.size() > 16U) return false;
    value = 0U;
    for (const char raw : text) {
        const unsigned char c = static_cast<unsigned char>(raw);
        std::uint64_t digit = 0U;
        if (c >= '0' && c <= '9') digit = static_cast<std::uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') digit = static_cast<std::uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = static_cast<std::uint64_t>(c - 'A' + 10);
        else return false;
        value = (value << 4U) | digit;
    }
    return true;
}

// A fixed-width, lowercase representation can be pasted back into Seed Entry
// without ambiguity or losing leading zeroes.
[[nodiscard]] inline std::string formatHexSeed(std::uint64_t seed) {
    constexpr char digits[] = "0123456789abcdef";
    std::string text = "0x";
    text.reserve(18U);
    for (int shift = 60; shift >= 0; shift -= 4) {
        text.push_back(digits[(seed >> static_cast<unsigned>(shift)) & 0x0fU]);
    }
    return text;
}
// Hex input is literal; other accepted text is normalized and hashed.
inline SeedTextParseResult parseSeedText(std::string_view input) {
    SeedTextParseResult result;
    const auto whitespace = [](unsigned char c) noexcept {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    };
    while (!input.empty() && whitespace(static_cast<unsigned char>(input.front()))) input.remove_prefix(1U);
    while (!input.empty() && whitespace(static_cast<unsigned char>(input.back()))) input.remove_suffix(1U);
    if (input.empty()) { result.error = "Enter a seed before starting."; return result; }
    if (input.size() > 64U) { result.error = "Seeds must be 1 to 64 characters long."; return result; }
    if (input.size() >= 2U && input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
        if (!parseHexSeed(input.substr(2U), result.seed)) {
            result.error = "Hex seeds need 1 to 16 hexadecimal digits after 0x."; return result;
        }
        result.valid = true; result.hexadecimal = true; result.normalizedText = "0x";
        for (char raw : input.substr(2U)) {
            const unsigned char c = static_cast<unsigned char>(raw);
            result.normalizedText.push_back(c >= 'A' && c <= 'F' ? static_cast<char>(c - 'A' + 'a') : raw);
        }
        return result;
    }
    if (input.size() == 16U) {
        std::uint64_t literal = 0U;
        if (parseHexSeed(input, literal)) {
            result.valid = true; result.hexadecimal = true; result.seed = literal; result.normalizedText = "0x";
            for (char raw : input) {
                const unsigned char c = static_cast<unsigned char>(raw);
                result.normalizedText.push_back(c >= 'A' && c <= 'F' ? static_cast<char>(c - 'A' + 'a') : raw);
            }
            return result;
        }
    }
    std::string normalized; normalized.reserve(input.size());
    for (char raw : input) {
        const unsigned char c = static_cast<unsigned char>(raw);
        if (whitespace(c)) {
            if (!normalized.empty() && normalized.back() != ' ') normalized.push_back(' ');
            continue;
        }
        char lowered = raw;
        if (c >= 'A' && c <= 'Z') lowered = static_cast<char>(c - 'A' + 'a');
        const unsigned char value = static_cast<unsigned char>(lowered);
        const bool allowed = (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_' || value == '-';
        if (!allowed) { result.error = "Use letters, numbers, spaces, underscores, or hyphens."; return result; }
        normalized.push_back(lowered);
    }
    while (!normalized.empty() && normalized.back() == ' ') normalized.pop_back();
    if (normalized.empty()) { result.error = "Enter a non-empty seed."; return result; }
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : normalized) { hash ^= static_cast<std::uint64_t>(c); hash *= 1099511628211ULL; }
    result.valid = true; result.seed = mixCourseBits(hash); result.normalizedText = normalized;
    return result;
}
} // namespace tunrun
