#include "cppasta/utf8.hpp"

namespace pasta {

bool isAscii(char ch)
{
    return ch > 0;
}

bool isContinuationByte(char ch)
{
    return (static_cast<uint8_t>(ch) & 0b11000000) == 0b10000000;
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

}
