#include <unordered_set>
#include <vector>

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch_test_macros.hpp>

#include <cppasta/dense_slot_map.hpp>
#include <cppasta/slot_map.hpp>

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

template <typename T>
auto collect(const T& sm)
{
    std::unordered_set<typename T::Value> r;
    auto key = sm.next({});
    while (key) {
        r.emplace(*sm.get(key));
        key = sm.next(key);
    }
    return r;
}

template <typename T>
auto collect_dense(const T& sm)
{
    std::vector<typename T::Value> r;
    for (const auto& v : sm) {
        r.push_back(v);
    }
    return r;
}

template <typename T>
auto map_keys(const T& sm, const std::vector<typename T::Key>& keys)
{
    std::vector<std::optional<typename T::Value>> r;
    for (const auto& k : keys) {
        const auto v = sm.find(k);
        r.push_back(v ? std::optional<typename T::Value>(*v) : std::nullopt);
    }
    return r;
}

template <typename T, typename Key>
using GrowableStorage = GrowableSlotMapStorage<T, Key, std::vector<uint32_t>, std::allocator>;

template <typename T, typename Key>
using PagedStorage = PagedSlotMapStorage<T, Key, std::vector<uint32_t>, std::allocator>;

template <typename S>
void test_slot_map()
{
    using C = std::unordered_set<std::string>;
    using O = std::vector<std::optional<std::string>>;

    S sm(4, 2);

    const auto foo = sm.insert("foo");
    REQUIRE(map_keys(sm, { foo }) == O { "foo" });
    REQUIRE(collect(sm) == C { "foo" });

    const auto bar = sm.insert("bar");
    REQUIRE(map_keys(sm, { foo, bar }) == O { "foo", "bar" });
    REQUIRE(collect(sm) == C { "foo", "bar" });

    REQUIRE(sm.remove(foo));
    const auto zap = sm.insert("zap");
    REQUIRE(map_keys(sm, { foo, bar, zap }) == O { {}, "bar", "zap" });
    REQUIRE(collect(sm) == C { "bar", "zap" });

    // insert requires resize
    const auto zip = sm.insert("zip");
    const auto zop = sm.insert("zop");
    const auto zep = sm.insert("zep");
    REQUIRE(map_keys(sm, { foo, bar, zap, zip, zop, zep })
        == O { {}, "bar", "zap", "zip", "zop", "zep" });
    REQUIRE(collect(sm) == C { "bar", "zap", "zip", "zop", "zep" });

    sm.resize(8);
    REQUIRE(map_keys(sm, { foo, bar, zap, zip, zop, zep })
        == O { {}, "bar", "zap", "zip", "zop", "zep" });
    REQUIRE(collect(sm) == C { "bar", "zap", "zip", "zop", "zep" });

    sm.clear();
    REQUIRE(map_keys(sm, { foo, bar, zap }) == O { {}, {}, {} });
    REQUIRE(collect(sm) == C {});
}

TEST_CASE("SlotMap<GrowableStorage>", "[slotmap]")
{
    test_slot_map<SlotMap<std::string, GrowableStorage>>();
}

TEST_CASE("SlotMap<GrowableStorage, Skipfield>", "[slotmap]")
{
    test_slot_map<SlotMap<std::string, GrowableStorage, CompositeId<std::string>,
        IntSkipfield<std::vector>>>();
}

TEST_CASE("SlotMap<PagedStorage>", "[slotmap]")
{
    test_slot_map<SlotMap<std::string, PagedStorage>>();
}

TEST_CASE("DenseSlotMap", "[slotmap]")
{
    using C = std::vector<std::string>;
    using O = std::vector<std::optional<std::string>>;

    DenseSlotMap<std::string, std::vector, std::vector> sm(6, 5);
    // insert elements
    const auto foo = sm.insert("foo");
    CAPTURE(foo);
    const auto bar = sm.insert("bar");
    CAPTURE(bar);
    const auto baz = sm.insert("baz");
    CAPTURE(baz);
    const auto bat = sm.insert("bat");
    CAPTURE(bat);
    const auto bla = sm.insert("bla");
    CAPTURE(bla);
    CAPTURE(sm.data());
    CAPTURE(sm.indices());

    REQUIRE(map_keys(sm, { foo, bar, baz, bat, bla }) == O { "foo", "bar", "baz", "bat", "bla" });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "baz", "bat", "bla" });

    // remove an element
    REQUIRE(sm.remove(baz));
    REQUIRE(!sm.remove(baz));
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(map_keys(sm, { foo, bar, baz, bat, bla }) == O { "foo", "bar", {}, "bat", "bla" });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "bla", "bat" });

    // remove an element from the same slot twice
    REQUIRE(sm.remove(bla));
    REQUIRE(!sm.remove(bla));
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(map_keys(sm, { foo, bar, baz, bat, bla }) == O { "foo", "bar", {}, "bat", {} });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "bat" });

    // reuse a slot
    const auto zop = sm.insert("zop");
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(
        map_keys(sm, { foo, bar, baz, bat, bla, zop }) == O { "foo", "bar", {}, "bat", {}, "zop" });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "bat", "zop" });
    REQUIRE(sm.size() == 4);
    CAPTURE(sm.free_head());

    // resize
    sm.reserve(8);
    CAPTURE(sm.free_head());
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(sm.capacity() == 8);
    REQUIRE(sm.size() == 4);
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(
        map_keys(sm, { foo, bar, baz, bat, bla, zop }) == O { "foo", "bar", {}, "bat", {}, "zop" });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "bat", "zop" });

    // fill up
    const auto zap = sm.insert("zap");
    CAPTURE(zap);
    CAPTURE(sm.free_head());
    const auto zep = sm.insert("zep");
    CAPTURE(zep);
    CAPTURE(sm.free_head());
    const auto zip = sm.insert("zip");
    CAPTURE(zip);
    CAPTURE(sm.free_head());
    const auto zup = sm.insert("zup");
    CAPTURE(zup);
    CAPTURE(sm.free_head());
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(sm.size() == 8);
    REQUIRE(sm.capacity() == 8);
    REQUIRE(map_keys(sm, { foo, bar, baz, bat, bla, zop, zip, zep, zap, zup })
        == O { "foo", "bar", {}, "bat", {}, "zop", "zip", "zep", "zap", "zup" });
    REQUIRE(collect_dense(sm) == C { "foo", "bar", "bat", "zop", "zap", "zep", "zip", "zup" });

    // insert requires resize
    const auto sup = sm.insert("sup");
    CAPTURE(sup);
    CAPTURE(sm.data());
    CAPTURE(sm.indices());
    REQUIRE(sm.contains(sup));
    REQUIRE(sm.size() == 9);
    REQUIRE(sm.capacity() == 13);
    REQUIRE(map_keys(sm, { foo, bar, baz, bat, bla, zop, zip, zep, zap, zup, sup })
        == O { "foo", "bar", {}, "bat", {}, "zop", "zip", "zep", "zap", "zup", "sup" });
    REQUIRE(
        collect_dense(sm) == C { "foo", "bar", "bat", "zop", "zap", "zep", "zip", "zup", "sup" });
}