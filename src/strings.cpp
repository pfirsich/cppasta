#include "cppasta/strings.hpp"

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
    try {
        size_t pos = 0;
        const auto val = std::stof(str, &pos);
        if (pos < str.size())
            return std::nullopt;
        return val;
    } catch (const std::exception& exc) {
        return std::nullopt;
    }
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
