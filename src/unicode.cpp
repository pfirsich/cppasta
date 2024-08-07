#include "cppasta/unicode.hpp"

#include <array>
#include <cassert>
#include <cstring>

#include <iostream>

namespace pasta {

bool is_valid_cp(Codepoint cp)
{
    if (cp > 0x10FFFF) // out of range for valid unicode codepoints
    {
        return false;
    }
    if (cp >= 0xD800 && cp <= 0xDFFF) // invalid utf-16 surrogate halves
    {
        return false;
    }
    if (cp >= 0xFDD0 && cp <= 0xFDEF) // non-characters
    {
        return false;
    }
    return true;
}

std::optional<std::pair<UnicodeEncoding, std::endian>> parse_bom(std::span<const uint8_t> data)
{
    if (data.size() >= 4 && std::memcmp(data.data(), "\xFF\xFE\x00\x00", 4) == 0) {
        return std::pair(UnicodeEncoding::Utf32, std::endian::little);
    } else if (data.size() >= 4 && std::memcmp(data.data(), "\x00\x00\xFE\xFF", 4) == 0) {
        return std::pair(UnicodeEncoding::Utf32, std::endian::little);
    } else if (data.size() >= 2 && std::memcmp(data.data(), "\xFF\xFE", 2) == 0) {
        return std::pair(UnicodeEncoding::Utf16, std::endian::little);
    } else if (data.size() >= 2 && std::memcmp(data.data(), "\xFE\xFF", 2) == 0) {
        return std::pair(UnicodeEncoding::Utf16, std::endian::big);
    } else if (data.size() >= 3 && std::memcmp(data.data(), "\xEF\xBB\xBF", 3) == 0) {
        return std::pair(UnicodeEncoding::Utf8, std::endian::native);
    }
    return std::nullopt;
}

namespace utf8 {
    bool is_continuation_byte(uint8_t byte)
    {
        return std::countl_one(byte) == 1; // 0b10xxxxxx
    }

    bool is_continuation_bytes(std::span<const uint8_t> data)
    {
        for (const auto byte : data) {
            if (!is_continuation_byte(byte)) {
                return false;
            }
        }
        return true;
    }

    std::optional<size_t> get_encoded_cp_length(uint8_t first_code_unit)
    {
        switch (std::countl_one(first_code_unit)) {
        case 0: // 0b0xxxxxxx - ascii
            return 1;
        case 2: // 0b110xxxxx
            return 2;
        case 3: // 0b1110xxxx
            return 3;
        case 4: // 0b11110xxx
            return 4;
        default:
            return std::nullopt;
        }
    }

    std::optional<size_t> get_cp_count(std::span<const uint8_t> data)
    {
        size_t c = 0;
        for (size_t i = 0; i < data.size();) {
            const auto l = get_encoded_cp_length(data[i]);
            if (!l || i + *l > data.size()) {
                return std::nullopt;
            }
            i += *l;
            c++;
        }
        return c;
    }

    std::optional<Codepoint> decode_cp(std::span<const uint8_t> data)
    {
        assert(data.size() <= 4);
        auto cont_bits = [](uint8_t ch) { return ch & 0b0011'1111; };

        switch (data.size()) {
        case 1:
            return data[0];
        case 2:
            return ((0b0001'1111 & data[0]) << 6) | cont_bits(data[1]);
        case 3:
            return ((0b0000'1111 & data[0]) << 12) | (cont_bits(data[1]) << 6) | cont_bits(data[2]);
        case 4:
            return ((0b0000'0111 & data[0]) << 18) | (cont_bits(data[1]) << 12)
                | (cont_bits(data[2]) << 6) | cont_bits(data[3]);
        default:
            return std::nullopt;
        }
    }

    std::optional<std::pair<Codepoint, size_t>> decode_first_cp(std::span<const uint8_t> data)
    {
        if (data.empty()) {
            return std::nullopt;
        }
        const auto l = get_encoded_cp_length(data[0]);
        if (!l || data.size() < *l) {
            return std::nullopt;
        }
        const auto cp_bytes = data.subspan(0, *l);
        if (!is_continuation_bytes(cp_bytes.subspan(1))) {
            return std::nullopt;
        }
        const auto cp = decode_cp(cp_bytes);
        if (!cp) {
            return std::nullopt;
        }
        return std::pair(*cp, *l);
    }

    bool is_valid_cp(Codepoint cp, size_t num_code_units)
    {
        if (!pasta::is_valid_cp(cp)) {
            return false;
        }

        static const std::array<Codepoint, 4> min { 0x00, 0x80, 0x800, 0x10000 };
        if (cp < min.at(num_code_units - 1)) // overlong encoding
        {
            return false;
        }

        return true;
    }

    bool is_valid(std::span<const uint8_t> data)
    {
        while (!data.empty()) {
            const auto res = decode_first_cp(data);
            if (!res) {
                return false;
            }
            const auto [cp, len] = *res;
            if (!is_valid_cp(cp, len)) {
                return false;
            }
            data = data.subspan(len);
        }
        return true;
    }

    std::optional<size_t> decode(
        std::span<const uint8_t> data, std::span<uint32_t> code_points, bool validate)
    {
        size_t num = 0;
        const auto res = decode(
            data,
            [&](uint32_t cp) {
                if (code_points.empty()) {
                    return false;
                }
                code_points[0] = cp;
                num++;
                code_points = code_points.subspan(1);
                return true;
            },
            validate);
        if (!res) {
            return std::nullopt;
        }
        return num;
    }

    std::optional<size_t> get_cp_length(Codepoint cp)
    {
        if (cp > 0x10FFFF) {
            return std::nullopt;
        } else if (cp >= 0x10000) {
            return 4;
        } else if (cp >= 0x800) {
            return 3;
        } else if (cp >= 0x80) {
            return 2;
        }
        return 1;
    }

    std::optional<size_t> encode_cp(Codepoint cp, std::span<uint8_t> buffer)
    {
        const auto len = get_cp_length(cp);
        if (!len) {
            return std::nullopt;
        }
        if (buffer.size() < *len) {
            return std::nullopt;
        }
        assert(*len <= 4);
        switch (*len) {
        case 1:
            assert(cp <= 0x7F);
            buffer[0] = static_cast<uint8_t>(cp);
            break;
        case 2:
            assert(cp <= 0x7FF);
            buffer[0] = 0b11000000 | static_cast<uint8_t>((cp >> 6) & 0b11'1111);
            buffer[1] = 0b10000000 | static_cast<uint8_t>((cp >> 0) & 0b11'1111);
            break;
        case 3:
            assert(cp <= 0xFFFF);
            buffer[0] = 0b11100000 | static_cast<uint8_t>((cp >> 12) & 0b11'1111);
            buffer[1] = 0b10000000 | static_cast<uint8_t>((cp >> 6) & 0b11'1111);
            buffer[2] = 0b10000000 | static_cast<uint8_t>((cp >> 0) & 0b11'1111);
            break;
        case 4:
            assert(cp <= 0x10FFFF);
            buffer[0] = 0b11110000 | static_cast<uint8_t>((cp >> 18) & 0b11'1111);
            buffer[1] = 0b10000000 | static_cast<uint8_t>((cp >> 12) & 0b11'1111);
            buffer[2] = 0b10000000 | static_cast<uint8_t>((cp >> 6) & 0b11'1111);
            buffer[3] = 0b10000000 | static_cast<uint8_t>((cp >> 0) & 0b11'1111);
            break;
        }
        return *len;
    }
}

namespace {
    uint16_t byteswap(uint16_t v)
    {
        const auto hi = v & 0xFF00;
        const auto lo = v & 0x00FF;
        return (lo << 8) | (hi >> 8);
    }

    uint16_t read_code_unit(std::span<const uint8_t> data, std::endian endianess)
    {
        assert(data.size() >= sizeof(uint16_t));
        uint16_t v = 0;
        std::memcpy(&v, data.data(), sizeof(uint16_t));
        if (endianess != std::endian::native) {
            v = byteswap(v);
        }
        return v;
    }

    void encode(uint16_t v, std::span<uint8_t> buffer, std::endian endianess)
    {
        assert(buffer.size() >= sizeof(uint16_t));
        if (endianess != std::endian::native) {
            v = byteswap(v);
        }
        std::memcpy(buffer.data(), &v, sizeof(uint16_t));
    }
}

namespace utf16 {

    CodeUnitType get_code_unit_type(uint16_t code_unit)
    {
        if (is_high_surrogate(code_unit)) {
            return CodeUnitType::HighSurrogate;
        }
        if (is_low_surrogate(code_unit)) {
            return CodeUnitType::LowSurrogate;
        }
        assert(is_bmp(code_unit));
        return CodeUnitType::BasicMultilingual;
    }

    bool is_high_surrogate(uint16_t cu)
    {
        return cu >= 0xD800 && cu <= 0xDBFF;
    }

    bool is_low_surrogate(uint16_t cu)
    {
        return cu >= 0xDC00 && cu <= 0xDFFF;
    }

    bool is_bmp(uint16_t cu)
    {
        return cu <= 0xD7FF || cu >= 0xE000;
    }

    std::optional<size_t> get_cp_length(std::span<const uint8_t> data, std::endian endianess)
    {
        if (data.size() < 2) {
            return std::nullopt;
        }
        const auto first = read_code_unit(data, endianess);
        if (is_bmp(first)) {
            return 2;
        }
        if (data.size() < 4) {
            return std::nullopt;
        }
        if (is_high_surrogate(first)) {
            const auto second = read_code_unit(data.subspan(2), endianess);
            if (is_low_surrogate(second)) {
                return 4;
            } else {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    std::optional<size_t> get_cp_count(std::span<const uint8_t> data, std::endian endianess)
    {
        size_t c = 0;
        for (size_t i = 0; i < data.size();) {
            const auto l = get_cp_length(data.subspan(i), endianess);
            if (!l || i + *l > data.size()) {
                return std::nullopt;
            }
            i += *l;
            c++;
        }
        return c;
    }

    Codepoint decode_surrogate_pair(uint16_t high, uint16_t low)
    {
        return 0x10000 + (((high - 0xD800) << 10) | (low - 0xDC00));
    }

    std::optional<std::pair<Codepoint, size_t>> decode_first_cp(
        std::span<const uint8_t> data, std::endian endianess)
    {
        if (data.size() < 2) {
            return std::nullopt;
        }
        const auto first = read_code_unit(data, endianess);
        if (is_bmp(first)) {
            return std::pair(static_cast<Codepoint>(first), 2);
        }
        if (data.size() < 4 || !is_high_surrogate(first)) {
            return std::nullopt;
        }
        const auto second = read_code_unit(data.subspan(2), endianess);
        if (!is_low_surrogate(second)) {
            return std::nullopt;
        }
        return std::pair(decode_surrogate_pair(first, second), 4);
    }

    bool is_valid(std::span<const uint8_t> data, std::endian endianess)
    {
        while (!data.empty()) {
            const auto res = decode_first_cp(data, endianess);
            if (!res) {
                return false;
            }
            const auto [cp, len] = *res;
            if (!is_valid_cp(cp)) {
                return false;
            }
            data = data.subspan(len);
        }
        return true;
    }

    std::optional<size_t> decode(std::span<const uint8_t> data, std::span<uint32_t> code_points,
        bool validate, std::endian endianess)
    {
        size_t num = 0;
        const auto res = decode(
            data,
            [&](uint32_t cp) {
                if (code_points.empty()) {
                    return false;
                }
                code_points[0] = cp;
                num++;
                code_points = code_points.subspan(1);
                return true;
            },
            validate, endianess);
        if (!res) {
            return std::nullopt;
        }
        return num;
    }

    std::pair<uint16_t, uint16_t> encode_surrogate_pair(Codepoint cp)
    {
        assert(cp >= 0x10000 && cp <= 0x10FFFF);
        const auto u = cp - 0x10000;
        assert(u <= 0xFFFFF); // u is a 20-bit value
        const auto high = static_cast<uint16_t>(0xD800 + (u >> 10));
        const auto low = static_cast<uint16_t>(0xDC00 + (u & 0b11'1111'1111));
        return std::pair(high, low);
    }

    std::optional<size_t> encode_cp(Codepoint cp, std::span<uint8_t> buffer, std::endian endianess)
    {
        assert(is_valid_cp(cp));
        if (cp <= 0xFFFF) {
            if (buffer.size() < 2) {
                return std::nullopt;
            }
            encode(static_cast<uint16_t>(cp), buffer, endianess);
            return 2;
        }
        const auto [high, low] = encode_surrogate_pair(cp);
        if (buffer.size() < 4) {
            return std::nullopt;
        }
        encode(high, buffer, endianess);
        encode(low, buffer.subspan(2), endianess);
        return 4;
    }
}

}