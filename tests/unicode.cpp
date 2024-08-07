#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch_test_macros.hpp>

#include <cppasta/unicode.hpp>

using namespace pasta;

CATCH_REGISTER_ENUM(
    UnicodeEncoding, UnicodeEncoding::Utf8, UnicodeEncoding::Utf16, UnicodeEncoding::Utf32);
CATCH_REGISTER_ENUM(std::endian, std::endian::little, std::endian::big);
CATCH_REGISTER_ENUM(utf16::CodeUnitType, utf16::CodeUnitType::HighSurrogate,
    utf16::CodeUnitType::LowSurrogate, utf16::CodeUnitType::BasicMultilingual);

template <typename... Args>
auto bytes(Args&&... args)
{
    return std::to_array<uint8_t>({ static_cast<uint8_t>(std::forward<Args>(args))... });
}

auto slice(std::span<const uint8_t> data, size_t num)
{
    assert(num <= data.size());
    return std::vector<uint8_t>(data.begin(), data.begin() + num);
}

struct VecAppender {
    std::vector<uint32_t> data;

    bool operator()(uint32_t v)
    {
        data.push_back(v);
        return true;
    }
};

TEST_CASE("is_valid_cp", "[unicode]")
{
    REQUIRE(is_valid_cp(0x20));
    REQUIRE(is_valid_cp(0x10FFFF));
    REQUIRE(!is_valid_cp(0x110000));
    REQUIRE(!is_valid_cp(0xD800));
}

TEST_CASE("parse_bom", "[unicode]")
{
    REQUIRE(parse_bom(bytes(0xEF, 0xBB, 0xBF))
        == std::pair(UnicodeEncoding::Utf8, std::endian::native));
    REQUIRE(parse_bom(bytes(0xFF, 0xFE)) == std::pair(UnicodeEncoding::Utf16, std::endian::little));
    REQUIRE(parse_bom(bytes(0x00)) == std::nullopt);
}

TEST_CASE("utf8::is_continuation_byte", "[unicode]")
{
    REQUIRE(utf8::is_continuation_byte(0x80));
    REQUIRE(!utf8::is_continuation_byte(0xC0));
}

TEST_CASE("utf8::is_continuation_bytes", "[unicode]")
{
    REQUIRE(utf8::is_continuation_bytes(bytes(0x80, 0x81)));
    REQUIRE(!utf8::is_continuation_bytes(bytes(0x80, 0xC0)));
}

TEST_CASE("utf8::get_encoded_cp_length", "[unicode]")
{
    REQUIRE(utf8::get_encoded_cp_length(0xF0) == 4);
    REQUIRE(utf8::get_encoded_cp_length(0xE0) == 3);
    REQUIRE(utf8::get_encoded_cp_length(0xC0) == 2);
    REQUIRE(utf8::get_encoded_cp_length(0x00) == 1);
}

TEST_CASE("utf8::get_cp_count", "[unicode]")
{
    REQUIRE(utf8::get_cp_count(bytes(0xF0, 0x90, 0x80, 0x80)) == 1);
    REQUIRE(utf8::get_cp_count(bytes(0xE2, 0x82, 0xAC, 0xC2, 0xA2)) == 2);
    REQUIRE(utf8::get_cp_count(bytes(0x00)) == 1);
}

TEST_CASE("utf8::decode_cp", "[unicode]")
{
    REQUIRE(utf8::decode_cp(bytes(0xE2, 0x82, 0xAC)) == 0x20AC);
    REQUIRE(utf8::decode_cp(bytes(0xF0, 0x9F, 0x92, 0xA9)) == 0x1F4A9);
}

TEST_CASE("utf8::decode_first_cp", "[unicode]")
{
    REQUIRE(
        utf8::decode_first_cp(bytes(0xE2, 0x82, 0xAC)) == std::pair<Codepoint, size_t>(0x20AC, 3));
    REQUIRE(utf8::decode_first_cp(bytes(0xF0, 0x9F, 0x92, 0xA9))
        == std::pair<Codepoint, size_t>(0x1F4A9, 4));
    REQUIRE(utf8::decode_first_cp(bytes(0x80, 0x80)) == std::nullopt);
}

TEST_CASE("utf8::is_valid_cp", "[unicode]")
{
    REQUIRE(utf8::is_valid_cp(0x20AC, 3));
    REQUIRE(!utf8::is_valid_cp(0x20AC, 4));
    REQUIRE(!utf8::is_valid_cp(0x110000, 4));
}

TEST_CASE("utf8::is_valid", "[unicode]")
{
    REQUIRE(utf8::is_valid(bytes(0xE2, 0x82, 0xAC)));
    REQUIRE(!utf8::is_valid(bytes(0xC0, 0x80)));
}

TEST_CASE("utf8::decode", "[unicode]")
{
    VecAppender appender1;
    REQUIRE(utf8::decode(bytes(0xE2, 0x82, 0xAC), appender1, true));
    REQUIRE(appender1.data == std::vector<uint32_t> { 0x20AC });

    VecAppender appender2;
    REQUIRE(!utf8::decode(bytes(0xC0, 0x80), appender2, true));

    VecAppender appender3;
    REQUIRE(utf8::decode(bytes(0xF0, 0x9F, 0x92, 0xA9), appender3, false));
    REQUIRE(appender3.data == std::vector<uint32_t> { 0x1F4A9 });

    std::array<uint32_t, 16> buf {};
    REQUIRE(utf8::decode(bytes(0xE2, 0x82, 0xAC), buf, true) == 1);
    REQUIRE(buf[0] == 0x20AC);

    REQUIRE(utf8::decode(bytes(0xC0, 0x80), buf, true) == std::nullopt);
}

TEST_CASE("utf8::get_cp_length", "[unicode]")
{
    REQUIRE(utf8::get_cp_length(0x20AC) == 3);
    REQUIRE(utf8::get_cp_length(0x1F4A9) == 4);
}

TEST_CASE("utf8::encode_cp", "[unicode]")
{
    std::array<uint8_t, 4> buf;

    REQUIRE(utf8::encode_cp(0x20AC, buf) == 3);
    REQUIRE(slice(buf, 3) == std::vector<uint8_t> { 0xE2, 0x82, 0xAC });

    REQUIRE(utf8::encode_cp(0x1F4A9, buf) == 4);
    REQUIRE(slice(buf, 4) == std::vector<uint8_t> { 0xF0, 0x9F, 0x92, 0xA9 });
}

TEST_CASE("utf16::get_code_unit_type", "[unicode]")
{
    REQUIRE(utf16::get_code_unit_type(0xD800) == utf16::CodeUnitType::HighSurrogate);
    REQUIRE(utf16::get_code_unit_type(0xDC00) == utf16::CodeUnitType::LowSurrogate);
    REQUIRE(utf16::get_code_unit_type(0x0061) == utf16::CodeUnitType::BasicMultilingual);
}

TEST_CASE("utf16::is_high_surrogate", "[unicode]")
{
    REQUIRE(utf16::is_high_surrogate(0xD800));
    REQUIRE(!utf16::is_high_surrogate(0x0061));
}

TEST_CASE("utf16::is_low_surrogate", "[unicode]")
{
    REQUIRE(utf16::is_low_surrogate(0xDC00));
    REQUIRE(!utf16::is_low_surrogate(0x0061));
}

TEST_CASE("utf16::is_bmp", "[unicode]")
{
    REQUIRE(utf16::is_bmp(0x0061));
    REQUIRE(!utf16::is_bmp(0xDC00));
}

TEST_CASE("utf16::get_cp_length", "[unicode]")
{
    REQUIRE(utf16::get_cp_length(bytes(0x00, 0x61), std::endian::big) == 2);
    REQUIRE(utf16::get_cp_length(bytes(0x61, 0x00), std::endian::little) == 2);
    REQUIRE(utf16::get_cp_length(bytes(0xD8, 0x00, 0xDC, 0x00), std::endian::big) == 4);
    REQUIRE(utf16::get_cp_length(bytes(0xD8, 0x00), std::endian::big) == std::nullopt);
}

TEST_CASE("utf16::get_cp_count", "[unicode]")
{
    REQUIRE(utf16::get_cp_count(bytes(0x00, 0x61, 0x00, 0x62), std::endian::big) == 2);
    REQUIRE(utf16::get_cp_count(bytes(0xD8, 0x00, 0xDC, 0x00), std::endian::big) == 1);
}

TEST_CASE("utf16::decode_surrogate_pair", "[unicode]")
{
    REQUIRE(utf16::decode_surrogate_pair(0xD800, 0xDC00) == 0x10000);
    REQUIRE(utf16::decode_surrogate_pair(0xDBFF, 0xDFFF) == 0x10FFFF);
}

TEST_CASE("utf16::decode_first_cp", "[unicode]")
{
    REQUIRE(utf16::decode_first_cp(bytes(0x61, 0x00), std::endian::little)
        == std::pair<Codepoint, size_t>(0x61, 2));
    REQUIRE(utf16::decode_first_cp(bytes(0x00, 0x61), std::endian::big)
        == std::pair<Codepoint, size_t>(0x61, 2));

    REQUIRE(utf16::decode_first_cp(bytes(0x00, 0xD8, 0x00, 0xDC), std::endian::little)
        == std::pair<Codepoint, size_t>(0x10000, 4));
    REQUIRE(utf16::decode_first_cp(bytes(0xD8, 0x00, 0xDC, 0x00), std::endian::big)
        == std::pair<Codepoint, size_t>(0x10000, 4));
}

TEST_CASE("utf16::is_valid", "[unicode]")
{
    REQUIRE(utf16::is_valid(bytes(0x00, 0x61), std::endian::big));
    REQUIRE(utf16::is_valid(bytes(0xD8, 0x00, 0xDC, 0x00), std::endian::big));
    REQUIRE(!utf16::is_valid(bytes(0xD8, 0x00), std::endian::big));
}

TEST_CASE("utf16::decode", "[unicode]")
{
    VecAppender appender1;
    REQUIRE(utf16::decode(bytes(0x00, 0x61), appender1, true, std::endian::big));
    REQUIRE(appender1.data == std::vector<uint32_t> { 0x0061 });

    VecAppender appender2;
    REQUIRE(utf16::decode(bytes(0xD8, 0x00, 0xDC, 0x00), appender2, true, std::endian::big));
    REQUIRE(appender2.data == std::vector<uint32_t> { 0x10000 });

    VecAppender appender3;
    REQUIRE(!utf16::decode(bytes(0xD8, 0x00), appender3, true, std::endian::big));

    std::array<uint32_t, 16> buf {};
    REQUIRE(utf16::decode(bytes(0x00, 0x61), buf, true, std::endian::big) == 1);
    REQUIRE(buf[0] == 0x61);

    REQUIRE(utf16::decode(bytes(0xD8, 0x00, 0xDC, 0x00), buf, true, std::endian::big) == 1);
    REQUIRE(buf[0] == 0x10000);
}

TEST_CASE("utf16::encode_surrogate_pair", "[unicode]")
{
    REQUIRE(utf16::encode_surrogate_pair(0x10000) == std::pair<uint16_t, uint16_t>(0xD800, 0xDC00));
    REQUIRE(
        utf16::encode_surrogate_pair(0x10FFFF) == std::pair<uint16_t, uint16_t>(0xDBFF, 0xDFFF));
}

TEST_CASE("utf16::encode_cp", "[unicode]")
{
    std::array<uint8_t, 4> buf;

    REQUIRE(utf16::encode_cp(0x61, buf, std::endian::little) == 2);
    REQUIRE(slice(buf, 2) == std::vector<uint8_t> { 0x61, 0x00 });

    REQUIRE(utf16::encode_cp(0x61, buf, std::endian::big) == 2);
    REQUIRE(slice(buf, 2) == std::vector<uint8_t> { 0x00, 0x61 });

    REQUIRE(utf16::encode_cp(0x10000, buf, std::endian::little) == 4);
    REQUIRE(slice(buf, 4) == std::vector<uint8_t> { 0x00, 0xD8, 0x00, 0xDC });

    REQUIRE(utf16::encode_cp(0x10000, buf, std::endian::big) == 4);
    REQUIRE(slice(buf, 4) == std::vector<uint8_t> { 0xD8, 0x00, 0xDC, 0x00 });
}