#pragma once

#include <filesystem>
#include <string>

#include <fmt/format.h>

template <>
struct fmt::formatter<std::filesystem::path> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx) const
    {
        const auto utf8Path = path.u8string();
        return format_to(ctx.out(), "{}",
            std::string(reinterpret_cast<const char*>(utf8Path.data()), utf8Path.size()));
    }
};
