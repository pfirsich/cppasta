#pragma once

#include <bit>
#include <cstdint>
#include <optional>
#include <span>

namespace pasta {

using Codepoint = uint32_t;

bool is_valid_cp(Codepoint cp);

enum class UnicodeEncoding { Utf8, Utf16, Utf32 };

// This will return std::endian::native for Utf8
std::optional<std::pair<UnicodeEncoding, std::endian>> parse_bom(std::span<const uint8_t> data);

namespace utf8 {
    bool is_continuation_byte(uint8_t byte);
    bool is_continuation_bytes(std::span<const uint8_t> data);

    // Returns the number of code units from the first code unit of a code point
    std::optional<size_t> get_encoded_cp_length(uint8_t first_code_unit);

    // Returns the number of code points in a buffer, but does not do any validation, just jumps
    // over each code point by the length derived from the first code unit.
    std::optional<size_t> get_cp_count(std::span<const uint8_t> data);

    // Decodes a code point from a buffer that already has the correct size of the code point.
    // This will not do any validation and just decode. You probably never want to use this without
    // wrapping or calling it from a more high level decoding function.
    std::optional<Codepoint> decode_cp(std::span<const uint8_t> data);

    // This function will check if the first code point is well-formed and will return it and it's
    // length in code units (i.e. bytes). You most likely want this function rather than the one
    // above. It will not validate the code point itself.
    std::optional<std::pair<Codepoint, size_t>> decode_first_cp(std::span<const uint8_t> data);

    // Calls utf32::is_valid_cp and checks that it's not an overlong encoding using
    // `num_code_units`.
    bool is_valid_cp(Codepoint cp, size_t num_code_units);

    // Decodes the whole string and validates each code point.
    bool is_valid(std::span<const uint8_t> data);

    // appender should take a Codepoint and return true if the value could be appended or false if
    // it could not.
    bool decode(
        std::span<const uint8_t> data, std::invocable<Codepoint> auto&& appender, bool validate)
    {
        while (!data.empty()) {
            const auto res = decode_first_cp(data);
            if (!res) {
                return false;
            }
            const auto [cp, len] = *res;
            data = data.subspan(len);

            if (validate && !is_valid_cp(cp, len)) {
                return false;
            }

            if (!appender(cp)) {
                return false;
            }
        }
        return true;
    }

    std::optional<size_t> decode(
        std::span<const uint8_t> data, std::span<uint32_t> code_points, bool validate);

    // Returns how many code units are required to encode a code point
    std::optional<size_t> get_cp_length(Codepoint cp);
    std::optional<size_t> encode_cp(Codepoint cp, std::span<uint8_t> buffer);
}

namespace utf16 {
    enum class CodeUnitType { HighSurrogate, LowSurrogate, BasicMultilingual };

    // If you get lost in a byte stream, you can use this to find the start of a code point.
    // If the code unit type is HighSurrogate or BasicMultilingual, you are at the start of a code
    // point, if it is LowSurrogate, the next code unit (16-bit word) should be the start of a new
    // code point.
    CodeUnitType get_code_unit_type(uint16_t code_unit);
    bool is_high_surrogate(uint16_t code_unit);
    bool is_low_surrogate(uint16_t code_unit);
    bool is_bmp(uint16_t code_unit);

    // Checks whether the beginning of data has a BMP code point or a high/low surrogate pair and
    // returns either 2 or 4 respectively. Or nullopt if pair is incomplete or invalid.
    std::optional<size_t> get_cp_length(
        std::span<const uint8_t> data, std::endian endianess = std::endian::native);

    // Calls get_cp_length in a loop for the whole data and returns the number of code points.
    std::optional<size_t> get_cp_count(
        std::span<const uint8_t> data, std::endian endianess = std::endian::native);

    // Doesn't do any validation for high and low
    Codepoint decode_surrogate_pair(uint16_t high, uint16_t low);

    // Returns the first code point data and its size in bytes.
    std::optional<std::pair<Codepoint, size_t>> decode_first_cp(
        std::span<const uint8_t> data, std::endian endianess = std::endian::native);

    // Calls decode_first_cp in a loop and checks is_valid_cp for each code point
    bool is_valid(std::span<const uint8_t> data, std::endian endianess = std::endian::native);

    // appender should take a Codepoint and return true if the value could be appended or false if
    // it could not.
    bool decode(std::span<const uint8_t> data, std::invocable<Codepoint> auto&& appender,
        bool validate, std::endian endianess = std::endian::native)
    {
        while (!data.empty()) {
            const auto res = decode_first_cp(data, endianess);
            if (!res) {
                return false;
            }
            const auto [cp, len] = *res;
            data = data.subspan(len);

            if (validate && !is_valid_cp(cp)) {
                return false;
            }

            if (!appender(cp)) {
                return false;
            }
        }
        return true;
    }

    std::optional<size_t> decode(std::span<const uint8_t> data, std::span<uint32_t> code_points,
        bool validate, std::endian endianess = std::endian::native);

    // Returns high and low surrogate
    std::pair<uint16_t, uint16_t> encode_surrogate_pair(Codepoint cp);

    // Encodes in host byte order1
    std::optional<size_t> encode_cp(
        Codepoint cp, std::span<uint8_t> buffer, std::endian endianess = std::endian::native);
}

}
