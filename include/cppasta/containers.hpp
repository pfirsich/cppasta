#include <cassert>
#include <optional>
#include <vector>

namespace pasta {

template <typename MapType, typename OutContainer = std::vector<typename MapType::key_type>>
OutContainer getKeys(const MapType& map)
{
    OutContainer keys;
    keys.reserve(map.size());
    for (const auto& elem : map) {
        keys.push_back(elem.first);
    }
    return keys;
}

template <typename MapType, typename OutContainer = std::vector<typename MapType::mapped_type>>
OutContainer getValues(const MapType& map)
{
    OutContainer vals;
    vals.reserve(map.size());
    for (const auto& elem : map) {
        vals.push_back(elem.second);
    }
    return vals;
}

template <typename InContainer, typename Func,
    typename OutContainer
    = std::vector<std::invoke_result_t<Func, typename InContainer::value_type>>>
OutContainer transform(const InContainer& in, Func&& func)
{
    OutContainer out;
    for (const auto& v : in) {
        out.push_back(func(v));
    }
    return out;
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
