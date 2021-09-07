#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pasta {

bool isAscii(char ch); // i.e. is single-byte code point
bool is4CodeUnitLeader(char c);
bool is2CodeUnitLeader(char c);
bool is3CodeUnitLeader(char c);
bool isContinuationByte(char ch);
// Simply returns the size of the code point according to it's first code unit
size_t getCodePointLength(char firstCodeUnit);
// This function also checks whether the following bytes (if the code point is > 1 unit)
// are adequately formed. If not, a shorter size it returned.
size_t getCodePointLength(std::string_view str);
std::optional<uint32_t> decode(std::string_view str);
std::optional<std::string> encode(uint32_t codePoint);

}
