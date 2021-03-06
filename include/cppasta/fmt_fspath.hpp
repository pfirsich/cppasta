#pragma once

#include <filesystem>

#include <fmt/format.h>

template <>
struct fmt::formatter<std::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", path.u8string());
    }
};
