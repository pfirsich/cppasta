#include <functional>
#include <iterator>
#include <type_traits>

namespace pasta {

// A bit like Boost's iterator_facade

template <typename Derived, typename ValueType, typename RefType = ValueType&,
    typename PointerType = ValueType*, typename DiffType = std::ptrdiff_t>
class Iterator {
    // Derived must have:
    // Derived::reference operator*();
    // Derived& operator++();
    // static_assert(std::is_copy_constructible_v<Derived>);
    // static_assert(std::is_copy_assignable_v<Derived>);
    // static_assert(std::is_destructible_v<Derived>);

public:
    using value_type = ValueType;
    using reference = RefType;
    using pointer = PointerType;
    using difference_type = DiffType;

    Derived operator++(int)
    {
        auto ret = derived();
        derived().operator++();
        return ret;
    }

private:
    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
};

template <typename Derived, typename ValueType, typename RefType = ValueType&,
    typename PointerType = ValueType*, typename DiffType = std::ptrdiff_t>
class InputIterator
    : public Iterator<InputIterator<Derived, ValueType, RefType, PointerType, DiffType>, ValueType,
          RefType, PointerType, DiffType> {
    // Derived must have:
    // Iterator
    // bool operator==(const Derived& other)
public:
    using value_type = ValueType;
    using reference = RefType;
    using pointer = PointerType;
    using difference_type = DiffType;

    template <typename U>
    bool operator!=(const U& other) const
    {
        return !(derived() == other);
    }

    pointer operator->() const
    {
        return &(*derived());
    }

private:
    const Derived& derived() const
    {
        return *static_cast<const Derived*>(this);
    }
    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
};

template <typename Derived, typename ValueType, typename RefType = ValueType&,
    typename PointerType = ValueType*, typename DiffType = std::ptrdiff_t>
class ForwardIterator
    : public InputIterator<ForwardIterator<Derived, ValueType, RefType, PointerType, DiffType>,
          ValueType, RefType, PointerType, DiffType> {
    // Derived must have:
    // InputIterator
    // static_assert(std::is_default_constructible_v<Derived>());

public:
    using value_type = ValueType;
    using reference = RefType;
    using pointer = PointerType;
    using difference_type = DiffType;
};

template <typename Derived, typename ValueType, typename RefType = ValueType&,
    typename PointerType = ValueType*, typename DiffType = std::ptrdiff_t>
class BidirectionalIterator
    : public ForwardIterator<
          BidirectionalIterator<Derived, ValueType, RefType, PointerType, DiffType>, ValueType,
          RefType, PointerType, DiffType> {
    // Derived must have:
    // ForwardIterator
    // operator--
public:
    using value_type = ValueType;
    using reference = RefType;
    using pointer = PointerType;
    using difference_type = DiffType;

    Derived operator--(int)
    {
        auto& ret = derived();
        derived().operator--();
        return ret;
    }

private:
    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
};

template <typename Derived, typename ValueType, typename RefType = ValueType&,
    typename PointerType = ValueType*, typename DiffType = std::ptrdiff_t>
class RandomAccessIterator
    : public BidirectionalIterator<
          RandomAccessIterator<Derived, ValueType, RefType, PointerType, DiffType>, ValueType,
          RefType, PointerType, DiffType> {
    // Derived must have:
    // BidirectionalIterator
    // Derived::difference operator-(const Derived& a, const Derived& b)
public:
    using value_type = ValueType;
    using reference = RefType;
    using pointer = PointerType;
    using difference_type = DiffType;

    Derived& operator+=(difference_type n)
    {
        if (n >= 0)
            while (n--)
                derived().operator++();
        else
            while (n++)
                derived().operator--();
        return derived();
    }

    Derived& operator-=(difference_type n)
    {
        return derived() += (-n);
    }

    Derived operator+(difference_type n) const
    {
        auto temp = derived();
        return temp += n;
    }

    Derived operator-(difference_type n) const
    {
        return derived() + (-n);
    }

    reference operator[](difference_type n) const
    {
        return *(derived() + n);
    }

    bool operator!=(const Derived& other) const
    {
        return !(derived() == other);
    }
    bool operator<(const Derived& other) const
    {
        return derived() - other > 0;
    }
    bool operator>(const Derived& other) const
    {
        return other < derived();
    }
    bool operator>=(const Derived& other) const
    {
        return !(derived() < other);
    }
    bool operator<=(const Derived& other) const
    {
        return !(derived() > other);
    }

private:
    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
    const Derived& derived() const
    {
        return *static_cast<const Derived*>(this);
    }
};

template <typename Container>
class IndexIterator
    : public RandomAccessIterator<IndexIterator<Container>, typename Container::value_type> {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename Container::value_type;
    using reference = typename Container::reference;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;

    IndexIterator(Container& container, size_t index = 0)
        : m_container(container)
        , m_index(index)
    {
    }

    IndexIterator(const IndexIterator& other) = default;
    IndexIterator& operator=(const IndexIterator& other) = default;

    IndexIterator& operator++()
    {
        ++m_index;
        return *this;
    }

    IndexIterator& operator--()
    {
        --m_index;
        return *this;
    }

    difference_type operator-(const IndexIterator& other) const
    {
        return m_index - other.m_index;
    }

    reference operator*() const
    {
        return m_container.get()[m_index];
    }

    bool operator==(const IndexIterator& other) const
    {
        return &m_container.get() == &other.m_container.get() && m_index == other.m_index;
    }

private:
    std::reference_wrapper<Container> m_container;
    size_t m_index;
};

template <typename Derived>
class IndexIterable {
public:
    auto begin()
    {
        return IndexIterator<Derived>(derived(), 0);
    }
    const auto begin() const
    {
        return IndexIterator<Derived>(derived(), 0);
    }
    auto end()
    {
        return IndexIterator<Derived>(derived(), derived().size());
    }
    const auto end() const
    {
        return IndexIterator<Derived>(derived(), derived().size());
    }

private:
    Derived& derived()
    {
        return *static_cast<Derived*>(this);
    }
    const Derived& derived() const
    {
        return *static_cast<const Derived*>(this);
    }
};

}
