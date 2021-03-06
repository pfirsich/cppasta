#pragma once

#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace pasta {

std::string hexString(const void* data, size_t size);

template <typename Container>
std::string hexString(const Container& container)
{
    return hexString(container.data(), container.size() * sizeof(typename Container::value_type));
}

// If Boost is available, use lexical_cast instead
template <typename T = long long>
std::optional<T> parseInt(const std::string& str, int base = 10)
{
    static constexpr auto min = std::numeric_limits<T>::min();
    static constexpr auto max = std::numeric_limits<T>::max();
    using IntMax = std::conditional_t<std::is_unsigned_v<T>, uintmax_t, intmax_t>;
    try {
        size_t pos = 0;
        IntMax val;
        if constexpr (std::is_unsigned_v<T>) {
            val = std::stoull(str, &pos, base);
        } else {
            val = std::stoll(str, &pos, base);
        }
        if (pos < str.size())
            return std::nullopt;
        if (val < min || val > max)
            return std::nullopt;
        return static_cast<T>(val);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<float> parseFloat(const std::string& str);

std::string toLower(std::string_view str);

// Split string between whitespace, e.g.: ("ab  cd") -> {"ab", "cd"}
std::vector<std::string> split(std::string_view str);

// Split string between delimiters, e.g.: ("ab  cd", ' ') -> {"ab", "", "cd"}
std::vector<std::string> split(std::string_view str, char delim);

template <typename Container>
std::string join(const Container& container, std::string_view delim)
{
    std::string concat;
    bool first = true;
    for (const auto& elem : container) {
        if (!first) {
            concat += delim;
        }
        first = false;
        concat += elem;
    }
    return concat;
}

bool startsWith(std::string_view str, std::string_view with);
bool endsWith(std::string_view str, std::string_view with);

}
