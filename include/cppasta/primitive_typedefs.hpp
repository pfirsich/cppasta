#pragma once

#include <cstdint>

// intentionally not in pasta namespace

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = u64;
using isize = i64;

using f32 = float;
using f64 = double;

consteval u8 operator""_u8(unsigned long long int v)
{
    return static_cast<u8>(v);
}

consteval u16 operator""_u16(unsigned long long int v)
{
    return static_cast<u16>(v);
}

consteval u32 operator""_u32(unsigned long long int v)
{
    return static_cast<u32>(v);
}

consteval u64 operator""_u64(unsigned long long int v)
{
    return static_cast<u64>(v);
}

consteval i8 operator""_i8(unsigned long long int v)
{
    return static_cast<i8>(v);
}

consteval i16 operator""_i16(unsigned long long int v)
{
    return static_cast<i16>(v);
}

consteval i32 operator""_i32(unsigned long long int v)
{
    return static_cast<i32>(v);
}

consteval i64 operator""_i64(unsigned long long int v)
{
    return static_cast<i64>(v);
}

consteval usize operator""_usize(unsigned long long int v)
{
    return static_cast<usize>(v);
}

consteval isize operator""_isize(unsigned long long int v)
{
    return static_cast<isize>(v);
}