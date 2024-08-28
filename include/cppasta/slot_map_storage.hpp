#pragma once

#include "generational_index.hpp"

namespace pasta {

template <typename S>
concept SlotMapStorage = requires(S s) {
    { s.store_free_list(std::declval<size_t>(), std::declval<uint32_t>()) } -> std::same_as<void>;
    {
        s.store_element(std::declval<size_t>(), std::declval<typename S::Element>())
    } -> std::same_as<void>;
    { s.destroy_element(std::declval<size_t>(), std::declval<uint32_t>()) } -> std::same_as<void>;
    { s.resize(std::declval<size_t>()) } -> std::same_as<void>;
    { std::as_const(s).size() } -> std::convertible_to<size_t>;
    { s.data(std::declval<size_t>()) } -> std::same_as<typename S::Element*>;
    { std::as_const(s).data(std::declval<size_t>()) } -> std::same_as<const typename S::Element*>;
    { std::as_const(s).free_list(std::declval<size_t>()) } -> std::same_as<uint32_t>;
    { s.gen(std::declval<size_t>()) } -> std::same_as<typename S::GenerationType&>;
    {
        std::as_const(s).gen(std::declval<size_t>())
    } -> std::same_as<const typename S::GenerationType&>;
};

template <typename T>
struct AlignedStorage {
    alignas(T) uint8_t data[sizeof(T)];
};

template <typename T>
struct ElementStorage {
    static constexpr size_t max(size_t a, size_t b) { return a > b ? a : b; }

    static constexpr size_t align = max(alignof(T), alignof(uint32_t));
    static constexpr size_t size = max(sizeof(T), sizeof(uint32_t));

    alignas(align) uint8_t data[size];
};

template <typename T, GenerationalIndex KeyType, typename GenerationStorage,
    template <typename> typename Allocator>
struct GrowableSlotMapStorage {
public:
    using GenerationType = KeyType::GenerationType;
    using Element = T;
    // TODO: Use alloc_traits if I need anything other than allocate/deallocate

    GrowableSlotMapStorage(size_t capacity, const Allocator<ElementStorage<T>>& alloc = {})
        : data_(alloc_.allocate(capacity))
        , generations_(capacity, 0)
        , alloc_(alloc)
    {
    }

    // Every element HAS TO BE DESTROYED before the destructor is called, or we will get undefined
    // behavior.
    ~GrowableSlotMapStorage() { alloc_.deallocate(data_, generations_.size()); }

    void resize(size_t size)
    {
        const auto new_data = alloc_.allocate(size);
        for (size_t i = 0; i < generations_.size(); ++i) {
            if (generations_[i] > 0) {
                new (new_data + i) T { std::move(*data(i)) };
                reinterpret_cast<T*>(data_ + i)->~T();
            } else {
                reinterpret_cast<uint32_t*>(data_ + i)->~uint32_t();
            }
        }
        alloc_.deallocate(data_, generations_.size());
        data_ = new_data;
        generations_.resize(size, 0);
    }

    void store_free_list(size_t idx, uint32_t free_list)
    {
        new (data_ + idx) uint32_t { free_list };
    }

    void store_element(size_t idx, T&& t)
    {
        assert(generations_[idx] == 0);
        reinterpret_cast<uint32_t*>(data_ + idx)->~uint32_t();
        new (data_ + idx) T { std::move(t) };
    }

    void destroy_element(size_t idx, uint32_t free_list)
    {
        assert(generations_[idx] > 0);
        reinterpret_cast<T*>(data_ + idx)->~T();
        new (data_ + idx) uint32_t { free_list };
    }

    auto size() const { return generations_.size(); }

    T* data(size_t idx) { return reinterpret_cast<T*>(data_ + idx); }
    const T* data(size_t idx) const { return reinterpret_cast<const T*>(data_ + idx); }

    uint32_t free_list(size_t idx) const
    {
        return *reinterpret_cast<const uint32_t*>(data_ + idx);
    };

    GenerationType& gen(size_t idx) { return generations_[idx]; }
    const GenerationType& gen(size_t idx) const { return generations_[idx]; }

private:
    ElementStorage<T>* data_;
    GenerationStorage generations_;
    Allocator<ElementStorage<T>> alloc_;
};

// This keeps element pointers stable and allocates in pages
template <typename T, GenerationalIndex KeyType, typename GenerationStorage,
    template <typename> typename Allocator>
struct PagedSlotMapStorage {
public:
    using GenerationType = KeyType::GenerationType;
    using Element = T;

    PagedSlotMapStorage(size_t capacity, const Allocator<ElementStorage<T>>& allocT = {},
        const Allocator<ElementStorage<T>*> allocP = {})
        : pages_(allocP_.allocate(1))
        , generations_(capacity, 0)
        , allocT_(allocT)
        , allocP_(allocP)
        , page_size_(capacity)
        , num_pages_(1)
    {
        pages_[0] = allocT_.allocate(page_size_);
    }

    // Every element HAS TO BE DESTROYED before the destructor is called, or we will get undefined
    // behavior.
    ~PagedSlotMapStorage()
    {
        for (size_t p = 0; p < num_pages_; ++p) {
            allocT_.deallocate(pages_[p], page_size_);
        }
        allocP_.deallocate(pages_, num_pages_);
    }

    // This ignores the argument and simply appends a new page every time
    void resize(size_t)
    {
        const auto new_page = allocT_.allocate(page_size_);
        const auto new_pages = allocP_.allocate(num_pages_);
        for (size_t i = 0; i < num_pages_; ++i) {
            new_pages[i] = pages_[i];
        }
        new_pages[num_pages_] = new_page;
        allocP_.deallocate(pages_, num_pages_);
        pages_ = new_pages;
        num_pages_++;
        generations_.resize(generations_.size() + page_size_, 0);
    }

    void store_free_list(size_t idx, uint32_t free_list) { new (data(idx)) uint32_t { free_list }; }

    void store_element(size_t idx, T&& t)
    {
        assert(generations_[idx] == 0);
        auto ptr = data(idx);
        reinterpret_cast<uint32_t*>(ptr)->~uint32_t();
        new (ptr) T { std::move(t) };
    }

    void destroy_element(size_t idx, uint32_t free_list)
    {
        assert(generations_[idx] > 0);
        auto ptr = data(idx);
        reinterpret_cast<T*>(ptr)->~T();
        new (ptr) uint32_t { free_list };
    }

    auto size() const { return generations_.size(); }

    T* data(size_t idx) { return reinterpret_cast<T*>(pages_[page_index(idx)] + elem_index(idx)); }

    const T* data(size_t idx) const
    {
        return reinterpret_cast<const T*>(pages_[page_index(idx)] + elem_index(idx));
    }

    uint32_t free_list(size_t idx) const
    {
        return *reinterpret_cast<const uint32_t*>(pages_[page_index(idx)] + elem_index(idx));
    };

    GenerationType& gen(size_t idx) { return generations_[idx]; }
    const GenerationType& gen(size_t idx) const { return generations_[idx]; }

private:
    size_t page_index(size_t idx) const { return idx / page_size_; }
    size_t elem_index(size_t idx) const { return idx % page_size_; }

    ElementStorage<T>** pages_;
    GenerationStorage generations_;
    Allocator<ElementStorage<T>> allocT_;
    Allocator<ElementStorage<T>*> allocP_;
    size_t page_size_;
    size_t num_pages_;
};

}