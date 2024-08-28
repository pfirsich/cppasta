#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace pasta {

template <typename T>
concept GenerationalIndex = requires(T id) {
    { id.idx() } -> std::unsigned_integral;
    { id.gen() } -> std::unsigned_integral;
    { id.valid() } -> std::convertible_to<bool>;
    { id.next_generation() } -> std::same_as<T>;
};

// CRTP Base
template <typename Derived>
struct GenerationalIdBase {
    static Derived max() { return Derived(-1, -1); }

    Derived next_generation() const
    {
        const auto g = (derived().gen() + 1) & max().gen();
        return Derived(derived().idx(), g == 0 ? 1 : g);
    }

    bool valid() const { return derived().gen() != 0; }

    explicit operator bool() const { return valid(); }

    const Derived& derived() const { return static_cast<const Derived&>(*this); }

    friend bool operator==(const Derived& a, const Derived& b)
    {
        return a.idx() == b.idx() && a.gen() == b.gen();
    }

    friend bool operator!=(const Derived& a, const Derived& b) { return !(a == b); }
};

template <typename Tag = void, typename BaseInt = uint64_t,
    size_t GenBits = sizeof(BaseInt) * 8 / 2>
class BitFieldId : public GenerationalIdBase<BitFieldId<Tag, BaseInt, GenBits>> {
public:
    static_assert(std::is_unsigned_v<BaseInt>);
    using IndexType = BaseInt;
    using GenerationType = BaseInt;

    BitFieldId() : index(0), generation(0) { }

    BitFieldId(IndexType i, GenerationType g) : index(i & IdxMask), generation(g & GenMask) { }

    IndexType idx() const { return index; }
    GenerationType gen() const { return generation; }

private:
    static constexpr auto IdxBits = sizeof(BaseInt) * 8 - GenBits;
    static constexpr BaseInt IdxMask = (1 << IdxBits) - 1;
    static constexpr BaseInt GenMask = (1 << GenBits) - 1;

    IndexType index : IdxBits;
    GenerationType generation : GenBits;
};

template <typename Tag = void, typename BaseInt = uint64_t,
    size_t GenBits = sizeof(BaseInt) * 8 / 2>
class BitMaskId : public GenerationalIdBase<BitMaskId<Tag, BaseInt, GenBits>> {
public:
    static_assert(std::is_unsigned_v<BaseInt>);
    using IndexType = BaseInt;
    using GenerationType = BaseInt;

    BitMaskId() : value(0) { }

    // The shift discards out-of-range bits, so that g will wrap around nicely
    BitMaskId(IndexType i, GenerationType g) : value((g << IdxBits) | (i & IdxMask)) { }

    IndexType idx() const { return value & IdxMask; }
    GenerationType gen() const { return value >> IdxBits; }

private:
    static constexpr auto IdxBits = sizeof(BaseInt) * 8 - GenBits;
    static constexpr BaseInt IdxMask = (1 << IdxBits) - 1;
    static constexpr BaseInt GenMask = (1 << GenBits) - 1;

    BaseInt value;
};

// https://quick-bench.com/q/JLeoe_LjQ3mEhId1gnbGFoZK20Q
// This is the fastest and simplest
template <typename Tag = void, typename IdxInt = uint32_t, typename GenInt = uint32_t>
struct CompositeId : public GenerationalIdBase<CompositeId<Tag, IdxInt, GenInt>> {
public:
    static_assert(std::is_unsigned_v<IdxInt>);
    static_assert(std::is_unsigned_v<GenInt>);
    using IndexType = IdxInt;
    using GenerationType = GenInt;

    CompositeId() : index(0), generation(0) { }
    CompositeId(IndexType i, GenerationType g) : index(i), generation(g) { }

    IndexType idx() const { return index; }
    GenerationType gen() const { return generation; }

private:
    IndexType index;
    GenerationType generation;
};
}