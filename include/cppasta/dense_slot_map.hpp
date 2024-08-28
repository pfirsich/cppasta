#pragma once

#include <cassert>

#include "generational_index.hpp"

/* DenseSlotMap

You probably know the pop-and-swap idiom, where instead of removing an element in the middle of a
sequence container like a vector, you swap the element to remove to the end and pop_back instead.
This saves expensive copying/moving of potentially many objects after the element that was removed.
Obviously this is only useful if the collection of objects is not ordered i.e. the order doesn't
matter. This container abstracts this idiom away and additionally provides functionality to
address items (via a key returned by insert). If you don't need to address elements later, just
use a vector and pop&swap ad-hoc, it's not worth it to abstract it away imho.

This container is even faster than the regular SlotMap for iteration (as fast as a vector), but
random access requires an additional indirection (the index into the data has to be looked up in
another container first) and you do not have stable addresses anymore, as objects are moved
around in the data storage (to fill gaps).

For data storage vector is probably a good default. Use deque if you want the slot map to grow
often and copying/moving T isn't cheap.

Use adapter for custom allocators:
    template <typename T> using Storage = std::vector<T, MyAllocator>;
*/

namespace pasta {

template <typename T, template <typename> typename DataStorage,
    template <typename> typename MetaStorage, GenerationalIndex KeyType = CompositeId<T>>
class DenseSlotMap {
public:
    using Key = KeyType;
    using Value = T;

    // If growth_constant and growth_factor are specified in such a way that growing the map will
    // not increase it's size (e.g. with the default arguments of 0 and 1.0 respectively), the map
    // will simply fail an assertion when it needs to resize and can't.
    DenseSlotMap(size_t capacity, size_t growth_constant = 0, float growth_factor = 1.0f)
        : indices_(capacity)
        , growth_constant_(growth_constant)
        , growth_factor_(growth_factor)
    {
        data_.reserve(capacity);
        backrefs_.reserve(capacity);
        // Initialize free list
        for (size_t i = 0; i < indices_.size(); ++i) {
            indices_[i] = Key(i + 1, 1);
        }
    }

    void reserve(size_t size)
    {
        data_.reserve(size);
        backrefs_.reserve(size);

        const auto old_size = indices_.size();
        indices_.resize(size);

        for (size_t i = old_size; i < size; ++i) {
            indices_[i] = Key(i + 1, 1);
        }

        const auto old_free_head = free_list_head_;
        free_list_head_ = old_size;
        indices_[size - 1] = Key(old_free_head, 1);
    }

    Key insert(const T& value) { return insert(T { value }); }

    Key insert(T&& value)
    {
        if (indices_.size() == data_.size()) {
            assert(indices_.size() == data_.size());
            const auto new_size
                = static_cast<size_t>(data_.size() * growth_factor_) + growth_constant_;
            assert(new_size > indices_.size() && "DenseSlotMap full");
            reserve(new_size);
        }

        assert(free_list_head_ < indices_.size());
        const auto indices_idx = free_list_head_;
        free_list_head_ = indices_[free_list_head_].idx();

        const auto data_idx = data_.size();
        data_.push_back(std::move(value));
        backrefs_.push_back(indices_idx);

        const auto gen = indices_[indices_idx].gen();
        indices_[indices_idx] = Key(data_idx, gen);
        return Key(indices_idx, gen);
    }

    bool remove(Key key)
    {
        const auto data_idx = indices_[key.idx()].idx();
        const auto last_idx = data_.size() - 1;

        if (!contains(key)) {
            return false;
        }

        // Swap and pop the data
        std::swap(data_[data_idx], data_[last_idx]);
        data_.pop_back();
        std::swap(backrefs_[data_idx], backrefs_[last_idx]);
        backrefs_.pop_back();

        // Update index for swapped element (last element before swap, now at data_idx) to point to
        // data_idx.
        const auto backref = backrefs_[data_idx];
        indices_[backref] = Key(data_idx, indices_[backref].gen());

        // Invalidate old key (increased stored generation) and put at the front of the free list
        const auto old_free_head = free_list_head_;
        free_list_head_ = key.idx();
        indices_[key.idx()] = Key(old_free_head, key.gen()).next_generation();

        return true;
    }

    // contains, find and get must only be called with keys previously returned by insert!

    bool contains(const Key& key) const
    {
        assert(key.idx() < indices_.size());
        return indices_[key.idx()].gen() == key.gen();
    }

    T* find(Key key) { return contains(key) ? get(key) : nullptr; }
    const T* find(Key key) const { return contains(key) ? get(key) : nullptr; }

    T* get(Key key)
    {
        assert(contains(key));
        return &data_[indices_[key.idx()].idx()];
    }

    const T* get(Key key) const
    {
        assert(contains(key));
        return &data_[indices_[key.idx()].idx()];
    }

    // Mostly useful if you want to remove during iteration or something.
    // ONLY WORKS FOR CONTIGUOUS STORAGE
    Key get_key(const T* element) const
    {
        const auto data_idx = element - data_.data();
        const auto indices_idx = backrefs_[data_idx];
        return Key(indices_idx, indices_[indices_idx].gen());
    }

    // There is no way to implement next in a way that makes it iterate fast (in the order of
    // data_), but still takes and returns a valid key without doing a bunch of extra work.
    // Just use begin/end/data.

    size_t size() const { return data_.size(); }
    size_t capacity() const { return indices_.size(); }

    auto begin() { return data_.begin(); }
    auto begin() const { return data_.begin(); }
    auto end() { return data_.end(); }
    auto end() const { return data_.end(); }

    // These are just for testing and debugging
    const auto& data() const { return data_; }
    const auto& indices() const { return indices_; }
    const auto& backrefs() const { return backrefs_; }
    auto free_head() const { return free_list_head_; }

private:
    // data_ has the actual data
    DataStorage<T> data_;
    /* indices_ is the map that maps key.idx() to an index for data_. It also stores the generation
       of the key pointing to that element in indices_.
       Essentially we want to increase the generation whenever we give out a key with the same index
       again, so we never give out keys twice (ignoring overflow). The generation for a given key
       index is just the one in `indices_[key.idx()].gen()`.

       If an element is unused and doesn't point to an element in data_, it will be used as a free
       list entry (see below).
    */
    MetaStorage<Key> indices_;
    /* If an element in the middle is removed, the last element is moved into that spot, meaning
       its index in data_ has changed. Of course indices_ needs to be updated to point to the new
       data index. Unfortunately there is no way to know which element in indices_ is pointing to a
       specific element in data, so we need another data structure.
       backref_ is always the same size as data_ and `backrefs_[i] = j` is simply an index to an
       entry in indices_ that points to `i`, so: `indices_[j].idx() = i`.
    */
    MetaStorage<size_t> backrefs_;
    /* indices_ is NOT dense (i.e. has gaps), so we need to reuse elements and therefore need to
       keep a free list. Instead of a separate data structure, we simply reuse unused indices_
       elements as free list entries. free_list_head_ will point to the first unused indices_ entry
       and that entry itself (idx()) will point to the next free entry.
    */
    size_t free_list_head_ = 0;
    size_t growth_constant_ = 0;
    float growth_factor_ = 0.0f;
};

}