#include <cassert>
#include <optional>
#include <vector>

namespace pasta {

template <typename MapType>
auto getKeys(const MapType& map)
{
    using KeyType = MapType::mapped_type;
    std::vector<KeyType> keys;
    keys.reserve(map.size());
    for (const auto& elem : map) {
        keys.push_back(elem.first);
    }
    return keys;
}

template <typename MapType>
auto getValues(const MapType& map)
{
    using ValueType = MapType::key_type;
    std::vector<ValueType> vals;
    vals.reserve(map.size());
    for (const auto& elem : map) {
        vals.push_back(elem.second);
    }
    return vals;
}

template <typename Index = size_t>
std::vector<Index> range(Index start, Index stop, Index step = 1)
{
    std::vector<Index> ret;
    assert(step != 0);
    if (step > 0) {
        assert(stop > start);
        for (auto i = start; i < stop; i += step) {
            ret.push_back(i);
        }
    } else {
        assert(stop < start);
        for (auto i = start; i > stop; i += step) {
            ret.push_back(i);
        }
    }
    return ret;
}

template <typename Index = size_t>
std::vector<Index> range(Index num)
{
    return range<Index>(0, num, 1);
}

template <typename Container, typename Value>
std::optional<std::ptrdiff_t> indexOf(const Container& container, const Value& findVal)
{
    auto it = std::find(container.begin(), container.end(), findVal);
    if (it == container.end())
        return std::nullopt;
    return std::distance(container.begin(), it);
}

}
