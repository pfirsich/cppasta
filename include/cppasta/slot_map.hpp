#pragma once

#include <cassert>

#include "generational_index.hpp"
#include "skipfield.hpp"
#include "slot_map_storage.hpp"

/* SlotMap

In games you often have unordered collections of things, which you frequently add to or remove from
(game objects/entities, particles, playing sounds, bullets).
Also in games usually everything references everything else (who played that sound, who shot that
bullet, etc.).

So it's common to require a collection that allows cheap removal and insertion with stable
references and fast traversal.

A linked list might do it, but as things disappear as often as they appear most references are
usually outdated, so pointers as references are bad. The next best and probably quite common
approach is a map of some sort with Ids as keys. I have used them a bunch, they are fine, but slot
maps should be faster.

Conceptually they also extend the map concept for the case where you don't really care about the
key. You don't really want to map one value to another, you just want to reference a value later. So
you simply let the map choose the key.

It has the following properties:
* If the slot map doesn't need to resize for insertion, lookup, removal and insertion are all O(1).
* Keys to removed values are invalidated using generational indices.
* Without resizing (or if PagedSlotMapStorage is used) pointers to entries are stable.
* The elements are stored in contiguous storage and fairly dense if the map is sufficiently full.

There are number of these on the internet in C++, many of which have not sufficiently thought about
iteration. The very best one, which I used as a reference for this is probably the one from
twiggler, but unfortunately it does not construct and destruct objects as they are inserted or
removed respectively. twiggler explicitly states this as a limitation and suggests to only use PODs,
but as you can see here, it's not too hard to get rid of this limitation. If it is supposed to be an
optimization, constructors for PODs are usually optimized to memcpy and destructors are NOPs.
Also I like the abstraction of a fully customizable Key type better instead of just customizing the
key parameters directly through the map.

Also they don't have a dense version of it, which I do (see dense_slot_map.hpp).

Implementation Notes:

Essentially this is a nice wrapper with generational indices around a sparse vector.
On insertion you give out a key that contains the index the new element was inserted in.

We need to increase the generation if we give out an index again and of course we also need to be
able if the generation that's part of a key corresponds to the element currently being stored in
that index. So we need to additionally store the current generation. Note that we start giving out
keys with generation 1, instead of zero, because we want a zero-initialized key (index=0, gen=0) to
be an invalid key.

So when a slot has a value, I need to store the value itself and the generation of that value.
When a slot does not have a value, I need to store the last generation.
I also need to store somewhere whether the value is present for a slot or not.

Additionally I need to store a free list of slots. Since I don't use the storage for the slot itself
while it is free, I will use that for the free list.

I could keep another map that stores whether a slot is in use to save the last generation, or I
simply use the generation to indicate occupancy (0 is empty, since that is invalid anyways) and keep
a global generation counter that I increase every time I give out a new key.

NOTE: If I chose to store occupancy separately, I could just make this an adapter von top of
SparseVector and a list of current generations, but then I can store the next generation per slot.
*/

namespace pasta {

// I realize all these template arguments are horrifying. I would probably never have it like this
// in a game, but since this is library code, I want it to be as customizable as possible, which it
// now is.
template <typename T, template <typename, typename> typename Storage,
    GenerationalIndex KeyType = CompositeId<T>, Skipfield Skipfield = NullSkipfield>
    requires SlotMapStorage<Storage<T, KeyType>>
class SlotMap {
public:
    using Key = KeyType;
    using Value = T;

    // The default arguments disallow growing
    SlotMap(size_t capacity, size_t growth_constant = 0, float growth_factor = 1.0f)
        : storage_(capacity)
        , skipfield_(capacity, true)
        , growth_constant_(growth_constant)
        , growth_factor_(growth_factor)
    {
        // Initialize free list
        for (size_t i = 0; i < storage_.size(); ++i) {
            storage_.store_free_list(i, i + 1);
        }
    }

    ~SlotMap() { clear(); }

    void resize(size_t size)
    {
        const auto old_size = storage_.size();
        storage_.resize(size);
        skipfield_.resize(size, true);
        for (size_t i = old_size; i < size - 1; ++i) {
            storage_.store_free_list(i, i + 1);
        }
        storage_.store_free_list(size - 1, free_list_head_);
        free_list_head_ = old_size;
    }

    Key insert(T&& value)
    {
        auto idx = free_list_head_;
        if (free_list_head_ >= storage_.size()) {
            // Space exhausted
            const auto new_size
                = static_cast<size_t>(storage_.size() * growth_factor_) + growth_constant_;
            assert(new_size > storage_.size() && "SlotMap full");
            resize(new_size);
        } else {
            assert(storage_.gen(free_list_head_) == 0);
            free_list_head_ = storage_.free_list(free_list_head_);
        }
        const auto key = Key(static_cast<Key::IndexType>(idx), generation_);
        generation_ = key.next_generation().gen();
        storage_.store_element(key.idx(), std::move(value));
        skipfield_.set_not_skipped(key.idx());
        storage_.gen(key.idx()) = key.gen();
        size_++;
        return key;
    }

    Key insert(const T& value) { return insert(T { value }); }

    bool remove(Key key)
    {
        const auto idx = key.idx();
        assert(idx < storage_.size());
        if (!key.valid() || key.gen() != storage_.gen(idx)) {
            return false;
        }
        storage_.destroy_element(idx, free_list_head_);
        free_list_head_ = idx;
        storage_.gen(idx) = 0;
        skipfield_.set_skipped(idx);
        size_--;
        return true;
    }

    bool contains(const Key& key) const
    {
        assert(key.idx() < storage_.size());
        return key.valid() && storage_.gen(key.idx()) == key.gen();
    }

    T* find(Key key) { return contains(key) ? get(key) : nullptr; }

    const T* find(Key key) const { return contains(key) ? get(key) : nullptr; }

    T* get(Key key)
    {
        assert(contains(key));
        return storage_.data(key.idx());
    }

    const T* get(Key key) const
    {
        assert(contains(key));
        return storage_.data(key.idx());
    }

    // Because you delete by key, next returns Key (also no const-overloading)
    Key next(Key key) const
    {
        auto i = key.valid() ? key.idx() + 1 : 0;
        i += static_cast<decltype(i)>(skipfield_.get_num_skipped(i));
        for (; static_cast<size_t>(i) < storage_.size(); ++i) {
            // If an actual skip field is used, the first iteration of this loop should return
            if (storage_.gen(i) > 0) {
                return Key(i, storage_.gen(i));
            }
        }
        return Key();
    }

    // Maybe this function would make sense (probably), I am not sure yet.
    // Id get_id(const T* elem) const;

    size_t size() const { return size_; }

    size_t capacity() const { return storage_.size(); }

    void clear()
    {
        for (size_t i = 0; i < storage_.size(); ++i) {
            if (storage_.gen(i) > 0) {
                remove(Key(i, storage_.gen(i)));
            }
        }
    }

private:
    Storage<T, Key> storage_;
    Skipfield skipfield_;
    size_t size_ = 0;
    size_t free_list_head_ = 0;
    size_t growth_constant_ = 0;
    float growth_factor_ = 1.0f;
    Key::GenerationType generation_ = 1;
};

}