#include "cppasta/strings.hpp"

#include <cstdlib>

namespace pasta {

std::string hexString(const void* data, size_t size)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    const auto iData = reinterpret_cast<const uint8_t*>(data);

    std::string out;
    out.reserve(size * 3);
    for (size_t i = 0; i < size; ++i) {
        const auto c = iData[i];
        out.push_back(hexDigits[c >> 4]);
        out.push_back(hexDigits[c & 15]);
    }
    return out;
}

std::optional<float> parseFloat(const std::string& str)
{
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    float val = 0.0f;
    const auto res
        = std::from_chars(str.data(), str.data() + str.size(), val, std::chars_format::general);
    if (res.ec != std::errc {}) {
        return std::nullopt;
    }
    if (res.ptr < str.data() + str.size()) {
        return std::nullopt;
    }
    return val;
#else
    // clang doesn't support from_chars for floats, after 6 years of it being in the standard, wtf
    char* strEnd = nullptr;
    const auto val = std::strtof(str.c_str(), &strEnd);
    if (val == HUGE_VALF || strEnd != str.c_str() + str.size()) {
        return std::nullopt;
    }
    return val;
#endif
}

std::string toLower(std::string_view str)
{
    std::string out;
    out.reserve(str.size());
    for (const auto ch : str)
        out.push_back(
            static_cast<char>(std::tolower(static_cast<int>(static_cast<unsigned char>(ch)))));
    return out;
}

std::vector<std::string> split(std::string_view str)
{
    std::vector<std::string> parts;
    // Welcome to clown town
    auto isWhitespace
        = [](char c) { return std::isspace(static_cast<int>(static_cast<unsigned char>(c))) != 0; };
    auto skip = [&str, &isWhitespace](size_t pos, bool skipWhitespace) {
        while (pos < str.size() && isWhitespace(str[pos]) == skipWhitespace)
            pos++;
        return pos;
    };
    size_t pos = 0;
    while (pos < str.size()) {
        pos = skip(pos, true);
        const auto len = skip(pos, false) - pos;
        if (len > 0)
            parts.emplace_back(str.substr(pos, len));
        pos += len;
    }
    return parts;
}

bool startsWith(std::string_view str, std::string_view with)
{
    return str.substr(0, with.size()) == with;
}

bool endsWith(std::string_view str, std::string_view with)
{
    return str.substr(str.size() - with.size()) == with;
}

}
