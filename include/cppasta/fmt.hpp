#pragma once

#include <fmt/format.h>

namespace pasta {

template <typename... Args>
void println(Args&&... args)
{
    fmt::print(std::forward<Args>(args)...);
    std::putc('\n', stdout);
    std::fflush(stdout);
}

template <typename... Args>
void println(std::FILE* f, Args&&... args)
{
    fmt::print(f, std::forward<Args>(args)...);
    std::putc('\n', f);
    std::fflush(f);
}

template <typename... Args>
void printErr(Args&&... args)
{
    println(stderr, std::forward<Args>(args)...);
}

}
