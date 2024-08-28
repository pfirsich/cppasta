#include <vector>

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch_test_macros.hpp>

#include <cppasta/skipfield.hpp>

using namespace pasta;

template <typename T>
std::vector<size_t> collect(const T& sf)
{
    std::vector<size_t> visited;
    size_t i = 0;
    while (i < sf.size()) {
        const auto s = sf.get_num_skipped(i);
        if (s == 0) {
            visited.push_back(i);
            i++;
        } else {
            i += s;
        }
    }
    return visited;
}

template <typename T>
std::vector<size_t> skipped(const T& sf)
{
    std::vector<size_t> r;
    for (size_t i = 0; i < sf.size(); ++i) {
        r.push_back(sf.get_num_skipped(i));
    }
    return r;
}

template <typename SF>
void test_common_set_not_skipped()
{
    using V = std::vector<size_t>;
    SF sf(8, true);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V {});

    // Test boundaries
    sf.set_not_skipped(0);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0 });

    sf.set_not_skipped(7);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 7 });

    // Test different cases
    sf.set_not_skipped(5); // left and right skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 5, 7 });

    sf.set_not_skipped(4); // left skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 4, 5, 7 });

    sf.set_not_skipped(1); // right skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 1, 4, 5, 7 });

    sf.set_not_skipped(2); // left and right not skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 1, 2, 4, 5, 7 });

    // clear remaining
    sf.set_not_skipped(3);
    sf.set_not_skipped(6);
    REQUIRE(collect(sf) == V { 0, 1, 2, 3, 4, 5, 6, 7 });

    sf.resize(12, false);
    REQUIRE(collect(sf) == V { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 });
}

template <typename SF>
void test_common_set_skipped()
{
    using V = std::vector<size_t>;
    SF sf(8, false);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 0, 1, 2, 3, 4, 5, 6, 7 });

    // Test boundaries
    sf.set_skipped(0);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 1, 2, 3, 4, 5, 6, 7 });

    sf.set_skipped(7);
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 1, 2, 3, 4, 5, 6 });

    // Test different cases
    sf.set_skipped(1); // right skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 2, 3, 4, 5, 6 });

    sf.set_skipped(6); // left skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 2, 3, 4, 5 });

    sf.set_skipped(3); // left and right skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 2, 4, 5 });

    sf.set_skipped(2); // left and right not skipped
    CAPTURE(skipped(sf));
    REQUIRE(collect(sf) == V { 4, 5 });

    sf.resize(12, true);
    REQUIRE(collect(sf) == V { 4, 5 });
}

TEST_CASE("IntSkipfield - set skipped", "[skipfield]")
{
    test_common_set_skipped<IntSkipfield<std::vector>>();
}

TEST_CASE("IntSkipfield - set not skipped", "[skipfield]")
{
    test_common_set_not_skipped<IntSkipfield<std::vector>>();
}

TEST_CASE("BoolSkipfield - set skipped", "[skipfield]")
{
    test_common_set_skipped<BoolSkipfield<std::vector>>();
}

TEST_CASE("BoolSkipfield - set not skipped", "[skipfield]")
{
    test_common_set_not_skipped<BoolSkipfield<std::vector>>();
}