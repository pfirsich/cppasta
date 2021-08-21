#pragma once

#include <algorithm>
#include <random>
#include <type_traits>

namespace pasta {

std::default_random_engine& getRng();

template <typename IntType>
std::enable_if_t<std::is_integral_v<IntType>, IntType> random(IntType min, IntType max)
{
    using DistType = std::uniform_int_distribution<IntType>;
    static DistType dist;
    return dist(getRng(), typename DistType::param_type(min, max));
}

template <typename FloatType>
std::enable_if_t<std::is_floating_point_v<FloatType>, FloatType> random(
    FloatType min, FloatType max)
{
    using DistType = std::uniform_real_distribution<FloatType>;
    static DistType dist;
    return dist(getRng(), typename DistType::param_type(min, max));
}

template <class, class Enable = void>
struct is_iterator : std::false_type {
};

template <typename T>
struct is_iterator<T,
    typename std::enable_if_t<std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<T>::iterator_category>::value
        || std::is_same<std::output_iterator_tag,
            typename std::iterator_traits<T>::iterator_category>::value>> : std::true_type {
};

template <typename Iterator>
std::enable_if_t<is_iterator<Iterator>::value, Iterator> random(Iterator begin, Iterator end)
{
    const auto size = std::distance(begin, end);
    if (size == 0)
        return begin;
    // MSVC does not allow uniform_int_distribution with size_t,
    // even though it does allow unsigned long long and both are uint64.
    return std::next(begin, random<unsigned long long>(0, size - 1));
}

template <typename Container>
auto random(Container&& container) -> decltype(*container.begin())
{
    return *random(
        std::forward<Container>(container).begin(), std::forward<Container>(container).end());
}

template <typename T>
T random();

template <>
inline float random()
{
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(getRng());
}

template <>
inline bool random()
{
    static std::uniform_int_distribution<int> dist(0, 1);
    return dist(getRng()) == 1;
}

template <typename Container>
void shuffle(Container&& container)
{
    std::shuffle(std::forward<Container>(container).begin(),
        std::forward<Container>(container).end(), getRng());
}

template <typename Container>
auto shuffled(const Container& container)
{
    auto copy = container;
    std::shuffle(copy.begin(), copy.end(), getRng());
    return copy;
}

}
