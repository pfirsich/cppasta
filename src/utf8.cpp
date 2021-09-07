#include "cppasta/utf8.hpp"

#include <algorithm>

namespace pasta {

namespace {
    constexpr auto b4CodeUnitsLeader = 0b11110'000;
    constexpr auto b3CodeUnitsLeader = 0b1110'0000;
    constexpr auto b2CodeUnitsLeader = 0b110'00000;
    constexpr auto bContinuationByte = 0b10'000000;
}

bool isAscii(char ch)
{
    return ch > 0;
}

bool is4CodeUnitLeader(char c)
{
    return (c & 0b11111'000) == b4CodeUnitsLeader;
}

bool is3CodeUnitLeader(char c)
{
    return (c & 0b1111'0000) == b3CodeUnitsLeader;
}

bool is2CodeUnitLeader(char c)
{
    return (c & 0b111'00000) == b2CodeUnitsLeader;
}

bool isContinuationByte(char c)
{
    return (c & 0b11000000) == bContinuationByte;
}

size_t getCodePointLength(char firstCodeUnit)
{
    const auto u = static_cast<uint8_t>(firstCodeUnit);
    if ((u & 0b11110000) == 0b11110000)
        return 4;
    if ((u & 0b11100000) == 0b11100000)
        return 3;
    if ((u & 0b11000000) == 0b11000000)
        return 2;
    else
        return 1;
}

size_t getCodePointLength(std::string_view str)
{
    if (str.empty()) {
        return 0;
    }
    const auto len = getCodePointLength(str[0]);
    for (size_t i = 1; i < len; ++i) {
        if (i >= str.size() || !isContinuationByte(str[i])) {
            return i;
        }
    }
    return len;
}

std::optional<uint32_t> decode(std::string_view str)
{
    auto contBits = [](char ch) { return ch & 0b00'111111; };
    switch (str.size()) {
    case 1:
        if (str[0] & 0b1'0000000) {
            return std::nullopt;
        }
        return static_cast<uint32_t>(str[0]);
        break;
    case 2:
        if (!is2CodeUnitLeader(str[0]) || !isContinuationByte(str[1])) {
            return std::nullopt;
        }
        return static_cast<uint32_t>(((str[0] & 0b000'11111) << 6) | contBits(str[1]));
        break;
    case 3:
        if (!is3CodeUnitLeader(str[0]) || !isContinuationByte(str[1])
            || !isContinuationByte(str[2])) {
            return std::nullopt;
        }
        return static_cast<uint32_t>(
            ((str[0] & 0b0000'1111) << 12) | (contBits(str[1]) << 6) | contBits(str[2]));
        break;
    case 4:
        if (!is4CodeUnitLeader(str[0]) || !isContinuationByte(str[1]) || !isContinuationByte(str[2])
            || !isContinuationByte(str[3])) {
            return std::nullopt;
        }
        return static_cast<uint32_t>(((str[0] & 0b00000'111) << 18) | (contBits(str[1]) << 12)
            | (contBits(str[2]) << 6) | contBits(str[3]));
        break;
    default:
        return std::nullopt;
    }
}

std::optional<std::string> encode(uint32_t codePoint)
{
    auto c = [](int n) { return static_cast<char>(n); };
    if (codePoint <= 0x7f) {
        return std::string(1, static_cast<char>(codePoint));
    } else if (codePoint <= 0x7ff) {
        char buf[2] = {
            c(b2CodeUnitsLeader | ((0b11111'000000 & codePoint) >> 6)),
            c(bContinuationByte | ((0b00000'111111 & codePoint) >> 0)),
        };
        return std::string(buf, 2);
    } else if (codePoint <= 0xffff) {
        char buf[3] = {
            c(b3CodeUnitsLeader | ((0b1111'000000'000000 & codePoint) >> 12)),
            c(bContinuationByte | ((0b0000'111111'000000 & codePoint) >> 6)),
            c(bContinuationByte | ((0b0000'000000'111111 & codePoint) >> 0)),
        };
        return std::string(buf, 3);
    } else if (codePoint <= 0x1fffff) {
        char buf[4] = {
            c(b4CodeUnitsLeader | ((0b111'000000'000000'000000 & codePoint) >> 18)),
            c(bContinuationByte | ((0b000'111111'000000'000000 & codePoint) >> 12)),
            c(bContinuationByte | ((0b000'000000'111111'000000 & codePoint) >> 6)),
            c(bContinuationByte | ((0b000'000000'000000'111111 & codePoint) >> 0)),
        };
        return std::string(buf, 4);
    } else {
        return std::nullopt;
    }
}
}
