#pragma once

#include <optional>

#include <fmt/format.h>

template <typename T>
struct fmt::formatter<std::optional<T>> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::optional<T>& opt, FormatContext& ctx)
    {
        if (opt)
            return format_to(ctx.out(), "{}", *opt);
        return format_to(ctx.out(), "nullopt");
    }
};
