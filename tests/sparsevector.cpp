#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch_test_macros.hpp>

#include <optional>

#include <cppasta/sparsevector.hpp>

using namespace pasta;

template <typename T>
std::vector<std::optional<T>> collect(const SparseVector<T>& v)
{
    std::vector<std::optional<T>> r;
    r.reserve(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        r.push_back(v.contains(i) ? std::optional<T>(v[i]) : std::nullopt);
    }
    return r;
}

template <typename T>
std::vector<bool> occupancy(const SparseVector<T>& v)
{
    std::vector<bool> r;
    r.reserve(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        r.push_back(v.contains(i));
    }
    return r;
}

TEST_CASE("sparsevector", "[sparsevector]")
{
    using Vec = std::vector<std::optional<std::string>>;
    const auto& null = std::nullopt;

    SparseVector<std::string> sparse(8);
    REQUIRE(collect(sparse) == Vec(sparse.size()));

    sparse.emplace(1, "foobar");
    sparse.emplace(5, "joel");
    sparse.emplace(6, "bazbaz");
    REQUIRE(collect(sparse) == Vec { null, "foobar", null, null, null, "joel", "bazbaz", null });
    REQUIRE(sparse.occupied() == 3);

    sparse[1] = "blub";
    REQUIRE(collect(sparse) == Vec { null, "blub", null, null, null, "joel", "bazbaz", null });
    REQUIRE(sparse.occupied() == 3);

    sparse.erase(5);
    REQUIRE(collect(sparse) == Vec { null, "blub", null, null, null, null, "bazbaz", null });
    REQUIRE(sparse.occupied() == 2);

    sparse.resize(12);
    REQUIRE(collect(sparse)
        == Vec { null, "blub", null, null, null, null, "bazbaz", null, null, null, null, null });
    REQUIRE(sparse.size() == 12);
    REQUIRE(sparse.occupied() == 2);
}
