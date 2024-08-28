#pragma once

#include <cassert>
#include <cstddef>

namespace pasta {

template <typename T>
concept Skipfield = requires(T sf) {
    { sf.set_skipped(std::declval<size_t>()) } -> std::same_as<void>;
    { sf.set_not_skipped(std::declval<size_t>()) } -> std::same_as<void>;
    { sf.get_num_skipped(std::declval<size_t>()) } -> std::unsigned_integral;
    { sf.resize(std::declval<size_t>(), std::declval<bool>()) } -> std::same_as<void>;
};

/* IntSkipfield

If you have a container that is occupied like this (X = occupied, 0 = empty):

i=0 1 2 3 4 5 6 7 8 9
o=X X X 0 0 0 X 0 X 0

a boolean skipfield would look like this:

o=0 0 0 1 1 1 0 1 0 1

which is essentially the inverse of the occupancy map.
If you wanted to iterate over only the occupied fields, you would loop through your container
and skip the element if the skip field contains a 1 at that position.

To increase iteration speed a bit the next step would then be to store the number of elements to
skip, so you can skip whole ranges in one go:

i=0 1 2 3 4 5 6 7 8 9
o=0 0 0 3 2 1 0 1 0 1

This is cool and seems easy, but usually the occupancy of elements changes frequently and
updating a skipfield like this is not very efficient.
E.g. if I you now wanted to mark i=6 as occupied, you would have to scan backwards through the
skipfield until you reach the start and end of a block and update everything to the left of that
newly marked element.

Consider the elements that changed value (essentially the whole block):

i=0 1 2 3 4 5 6 7 8 9
o=0 0 0 5 4 3 2 1 0 1

We have to rewrite the values anyways if we connect two skipped blocks, but we can save having to
scan for the beginning of the range by storing the length of the run at the end of the block, like
so:

i=0 1 2 3 4 5 6 7 8 9
o=0 0 0 3 2 3 0 1 0 1 # before marking i=6 skipped
o=0 0 0 5 4 3 2 5 0 1 # after marking i=6 skipped

This way you know where the skip block begins by looking at the value to the left of the element
being marked and jumping back this many elements minus 1 without branches (except to distinguish the
case).

Since you won't be marking a skipped block skipped again, you really only have to look at the two
cases of a skipped block being to the left or to the right of an element being marked (or both) and
with this approach you can find the bounds of the skip block in constant time.

I we somehow didn't care about the values between the start and end of a skip block, which is
usually the case for iteration, we could ignore all the elements inside the block and we could get
away with *updating* the skipped block in constant time as well! Unfortunately you don't just want
to mark unskipped elements as skipped, but skipped elements as unskipped and in that case you also
need to know the skip block bounds from anywhere inside it, so you can split it.
So we need to keep the decreasing run of numbers between a skip block (sad).

There is also a pattern where you store an increasing sequence of numbers between a block, starting
at 2, but I do not understand what that would help with. Imho it has the same number of cases that
require rewriting the whole block and the same number of cases that don't.


*/

// NOTE: You must not modify IntSkipfield during iteration!
template <template <typename> typename Storage>
class IntSkipfield {
public:
    IntSkipfield(size_t size, bool init_skipped) : num_skipped_(size, 0)
    {
        if (init_skipped) {
            set_range_skipped(0, size);
        }
    }

    void resize(size_t size, bool init_skipped)
    {
        assert(size > num_skipped_.size());
        const auto old_size = num_skipped_.size();
        num_skipped_.resize(size, 0);
        if (init_skipped) {
            if (old_size == 0 || num_skipped_[old_size - 1] == 0) {
                set_range_skipped(old_size, size - old_size);
                return;
            }
            const auto prev = num_skipped_[old_size - 1];
            set_range_skipped(old_size - prev, size - old_size + prev);
        }
    }

    void set_skipped(size_t idx)
    {
        assert(idx < num_skipped_.size());
        assert(num_skipped_[idx] == 0);
        const auto prev = idx > 0 ? num_skipped_[idx - 1] : 0;
        const auto next = idx + 1 < num_skipped_.size() ? num_skipped_[idx + 1] : 0;

        if (prev == 0 && next == 0) {
            num_skipped_[idx] = 1;
        } else if (prev == 0 && next != 0) {
            // A skipped range to the right of idx
            // before: 0 0 X 4 3 2 4 0 0 0
            // after:  0 0 5 4 3 2 5 0 0 0
            num_skipped_[idx] = next + 1;
            num_skipped_[idx + next - 1] = next + 1;
        } else if (prev != 0 && next == 0) {
            // A skipped range to the left of idx
            // before: 0 0 0 4 3 2 4 X 0 0
            // after:  0 0 0 5 4 3 2 5 0 0
            set_range_skipped(idx - prev, prev + 1);
        } else if (prev != 0 && next != 0) {
            // A skipped range on both sides
            // before: 0 2 2 X 2 2 0
            // after:  0 5 4 3 2 5 0
            set_range_skipped(idx - prev, prev + next + 1);
        }
    }

    void set_not_skipped(size_t idx)
    {
        assert(idx < num_skipped_.size());
        const auto cur = num_skipped_[idx];
        assert(cur > 0);
        const auto prev = idx > 0 ? num_skipped_[idx - 1] : 0;
        const auto next = idx + 1 < num_skipped_.size() ? num_skipped_[idx + 1] : 0;

        if (prev == 0 && next == 0) {
            num_skipped_[idx] = 0;
        } else if (prev == 0 && next != 0) {
            // A skipped range to the right of idx
            // before: 0 0 X 4 3 2 5 0 0 0
            // after:  0 0 0 4 3 2 4 0 0 0
            num_skipped_[idx] = 0;
            num_skipped_[idx + cur - 1] = cur - 1;
        } else if (prev != 0 && next == 0) {
            // A skipped range to the left of idx
            // before: 0 0 0 5 4 3 2 X 0 0
            // after:  0 0 0 4 3 2 4 0 0 0
            const auto new_len = cur - 1;
            num_skipped_[idx] = 0;
            set_range_skipped(idx - new_len, new_len);
        } else if (prev != 0 && next != 0) {
            // A skipped range on both sides
            // before: 0 5 4 3 2 5 0
            // after:  0 2 2 0 2 2 0
            const auto last = idx + cur - 1;
            const auto len = num_skipped_[last];
            const auto first = last + 1 - len;
            num_skipped_[idx] = 0;
            set_range_skipped(first, idx - first);
            set_range_skipped(idx + 1, last - idx);
        }
    }

    size_t get_num_skipped(size_t idx) const
    {
        assert(idx < num_skipped_.size());
        return num_skipped_[idx];
    }

    size_t size() const { return num_skipped_.size(); }

private:
    void set_range_skipped(size_t start, size_t count)
    {
        for (size_t i = 0; i < count - 1; ++i) {
            num_skipped_[start + i] = count - i;
        }
        num_skipped_[start + count - 1] = count;
    }

    Storage<uint32_t> num_skipped_;
};

// This is mostly useful if you want to modify the skipfield during iteration.
template <template <typename> typename Storage>
class BoolSkipfield {
public:
    BoolSkipfield(size_t size, bool init_skipped) : skipped_(size, init_skipped ? 1 : 0) { }

    void resize(size_t size, bool init_skipped)
    {
        assert(size > skipped_.size());
        skipped_.resize(size, init_skipped ? 1 : 0);
    }

    void set_skipped(size_t idx)
    {
        assert(idx < skipped_.size());
        assert(skipped_[idx] == 0);
        skipped_[idx] = 1;
    }

    void set_not_skipped(size_t idx)
    {
        assert(idx < skipped_.size());
        assert(skipped_[idx] > 0);
        skipped_[idx] = 0;
    }

    size_t get_num_skipped(size_t idx) const
    {
        assert(idx < skipped_.size());
        size_t i = 0;
        while (idx + i < skipped_.size() && skipped_[idx + i] > 0) {
            i++;
        }
        return i;
    }

    size_t size() const { return skipped_.size(); }

private:
    Storage<uint8_t> skipped_; // vector<bool> is evil
};

struct NullSkipfield {
    NullSkipfield(size_t, bool) { }
    void resize(size_t, bool) { }
    void set_skipped(size_t) { }
    void set_not_skipped(size_t) { }
    size_t get_num_skipped(size_t) const { return 0; }
};

}