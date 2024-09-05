#pragma once

#include <charconv>
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
std::optional<T> parseInt(std::string_view str, int base = 10)
{
    constexpr auto min = std::numeric_limits<T>::min();
    constexpr auto max = std::numeric_limits<T>::max();

    std::conditional_t<std::is_unsigned_v<T>, uintmax_t, intmax_t> val = 0;
    const auto res = std::from_chars(str.data(), str.data() + str.size(), val, base);
    if (res.ec != std::errc {}) {
        return std::nullopt;
    }
    if (res.ptr < str.data() + str.size()) {
        return std::nullopt;
    }
    if (val < min || val > max) {
        return std::nullopt;
    }
    return static_cast<T>(val);
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
