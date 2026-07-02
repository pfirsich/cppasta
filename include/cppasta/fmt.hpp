#pragma once

#include <cstdio>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace pasta {

inline void println(std::FILE* f, std::string_view str)
{
    std::fwrite(str.data(), 1, str.size(), f);
    std::putc('\n', f);
    std::fflush(f);
}

inline void println(std::string_view str)
{
    println(stdout, str);
}

template <typename... Args>
requires(sizeof...(Args) > 0)
void println(fmt::format_string<Args...> format, Args&&... args)
{
    fmt::print(format, std::forward<Args>(args)...);
    std::putc('\n', stdout);
    std::fflush(stdout);
}

template <typename... Args>
requires(sizeof...(Args) > 0)
void println(std::FILE* f, fmt::format_string<Args...> format, Args&&... args)
{
    fmt::print(f, format, std::forward<Args>(args)...);
    std::putc('\n', f);
    std::fflush(f);
}

inline void printErr(std::string_view str)
{
    println(stderr, str);
}

template <typename... Args>
requires(sizeof...(Args) > 0)
void printErr(fmt::format_string<Args...> format, Args&&... args)
{
    ::pasta::println(stderr, format, std::forward<Args>(args)...);
}

}
