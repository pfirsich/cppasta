#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch_test_macros.hpp>

#include <cppasta/generational_index.hpp>

using namespace pasta;

namespace Catch {
template <GenerationalIndex Id>
struct StringMaker<Id> {
    static std::string convert(const Id& v)
    {
        return "{" + std::to_string(v.idx()) + ", " + std::to_string(v.gen()) + "}";
    }
};
}

template <typename Id>
void test()
{
    REQUIRE(!Id());
    REQUIRE(Id(42, 69));
    REQUIRE(Id(42, 69).idx() == 42);
    REQUIRE(Id(42, 69).gen() == 69);
    REQUIRE(Id(42, 69) == Id(42, 69));
    REQUIRE(Id(42, 69) != Id(1, 69));
    REQUIRE(Id(42, 69) != Id(42, 1));
    REQUIRE(Id::max() == Id(255, 255));
    REQUIRE(Id(42, 69).next_generation() == Id(42, 70));
    REQUIRE(Id(42, 255).next_generation() == Id(42, 1));
}

TEST_CASE("BitFieldId", "[generational_index]")
{
    test<BitFieldId<void, uint16_t, 8>>();
}

TEST_CASE("BitMaskId", "[generational_index]")
{
    test<BitMaskId<void, uint16_t, 8>>();
}

TEST_CASE("CompositeId", "[generational_index]")
{
    test<CompositeId<void, uint8_t, uint8_t>>();
}