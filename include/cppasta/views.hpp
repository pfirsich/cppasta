#include <cassert>
#include <random>
#include <vector>

#include "iterators.hpp"

namespace pasta {

/*
 * These views can not be views over temporaries (that includes other views!)
 * see here: https://en.cppreference.com/w/cpp/language/range-for
 * in the case of:
 * for(auto x : RandomView(std::vector<int>{1, 2, 3, 4}))
 * this would happen:
 * auto&& __range = RandomView(std::vector<int>{1, 2, 3, 4});
 * only RandomView has it's lifetime extended and std::vector gets destroyed at the ;
 *
 * One could silently copy rvalues, but I think this is too expensive to do silently.
 * Views could copy other views though, so they could be silently copied, but this is
 * too complicated for now.
 */

template <typename C, typename T>
struct transfer_const {
    using type = T;
};

template <typename C, typename T>
struct transfer_const<const C, T> {
    using type = const T;
};

template <typename C, typename T>
using transfer_const_t = typename transfer_const<C, T>::type;

template <typename Container>
class RangeView : public IndexIterable<RangeView<Container>> {
public:
    using value_type = typename Container::value_type;
    using reference = transfer_const_t<Container, typename Container::value_type>&;

    RangeView(Container& container, size_t offset, size_t size)
        : m_container(container)
        , m_offset(offset)
        , m_size(size)
    {
        assert(m_offset < m_container.size() && m_offset + m_size <= m_container.size());
    }

    reference operator[](size_t index) const
    {
        assert(index < m_size);
        return m_container[m_offset + index];
    }

    auto size() const
    {
        return m_size;
    }

private:
    Container& m_container;
    size_t m_offset;
    size_t m_size;
};

template <typename Container>
class RandomView : public IndexIterable<RandomView<Container>> {
public:
    using value_type = typename Container::value_type;
    using reference = transfer_const_t<Container, typename Container::value_type>&;

    RandomView(Container& container)
        : m_container(container)
    {
        static std::random_device seed;
        static std::default_random_engine rng(seed());
        m_indices.reserve(m_container.size());
        for (size_t i = 0; i < m_container.size(); ++i)
            m_indices.push_back(i);
        std::shuffle(m_indices.begin(), m_indices.end(), rng);
    }

    reference operator[](size_t index) const
    {
        assert(index < m_indices.size());
        assert(m_container.size() == m_indices.size());
        return m_container[m_indices[index]];
    }

    auto size() const
    {
        return m_container.size();
    }

private:
    Container& m_container;
    std::vector<size_t> m_indices;
};

template <typename Container>
class MatrixView : public IndexIterable<MatrixView<Container>> {
public:
    using value_type = RangeView<Container>;
    using reference = transfer_const_t<Container, RangeView<Container>>;

    MatrixView(Container& container, size_t rows, size_t columns)
        : m_container(container)
        , m_rows(rows)
        , m_columns(columns)
    {
        assert(m_rows * m_columns == m_container.size());
    }

    reference operator[](size_t index) const
    {
        LOG_ASSERT(index < m_rows, "Index (index = {}) out of range (rows = {}) for MatrixView",
            index, m_rows);
        return RangeView<Container>(m_container, index * m_columns, m_columns);
    }

    auto size() const
    {
        return m_rows;
    }
    auto dimensions() const
    {
        return std::pair(m_rows, m_columns);
    }

private:
    Container& m_container;
    size_t m_rows, m_columns;
};

template <typename Container>
class EnumerationView : public IndexIterable<EnumerationView<Container>> {
public:
    using value_type = std::pair<size_t, typename Container::value_type>;
    using reference
        = std::pair<size_t, transfer_const_t<Container, typename Container::value_type>&>;

    EnumerationView(Container& container)
        : m_container(container)
    {
    }

    reference operator[](size_t index) const
    {
        LOG_ASSERT(index < size(), "Index ({}) out of range ({})", index, size());
        return reference(index, m_container[index]);
    }

    auto size() const
    {
        return m_container.size();
    }

private:
    Container& m_container;
};

/*template <typename Container, typename Predicate>
class FilterView : public IndexIterable<FilterView<Container, Predicate>> {
public:
    using value_type = typename Container::value_type;
    using reference = transfer_const_t<Container, value_type>&;
    static_assert();

    FilterView(Container& container, Predicate filterFunc)
        : m_container(container)
        , m_pred(filterFunc)
    {
    }

private:
    Container& m_container;
    std::function m_pred;
};*/

}
