#pragma once

#include <cassert>
#include <type_traits>
#include <vector>

namespace pasta {

template <typename T>
class SparseVector {
public:
    SparseVector() = default;

    SparseVector(size_t size)
        : data_(alloc(size))
        , size_(size)
        , occupied_(size, 0)
    {
    }

    ~SparseVector()
    {
        for (size_t i = 0; i < size_; ++i) {
            if (occupied_[i]) {
                erase(i);
            }
        }
        operator delete(data_, std::align_val_t(alignof(T)));
    }

    size_t size() const
    {
        return size_;
    }

    size_t occupied() const
    {
        return numOccupied_;
    }

    void resize(size_t size)
    {
        assert(size > size_);
        auto newData = alloc(size);
        for (size_t i = 0; i < size_; ++i) {
            if (occupied_[i]) {
                new (newData + i) T { std::move(data_[i]) };
                data_[i].~T();
            }
        }
        operator delete(data_, std::align_val_t(alignof(T)));
        data_ = newData;
        size_ = size;
        occupied_.resize(size, 0);
    }

    void insert(size_t index, const T& v)
    {
        emplace(index, v);
    }

    void insert(size_t index, T&& v)
    {
        emplace(index, std::move(v));
    }

    template <typename... Args>
    T& emplace(size_t index, Args&&... args)
    {
        assert(index < size_);
        assert(!contains(index));
        new (&data_[index]) T { std::forward<Args>(args)... };
        occupied_[index] = 1;
        numOccupied_++;
        return data_[index];
    }

    bool contains(size_t index) const
    {
        return index < occupied_.size() && occupied_[index];
    }

    void erase(size_t index)
    {
        assert(contains(index));
        data_[index].~T();
        occupied_[index] = 0;
        numOccupied_--;
    }

    const T& get(size_t index) const
    {
        assert(contains(index));
        return data_[index];
    }

    T& get(size_t index)
    {
        assert(contains(index));
        return data_[index];
    }

    T& operator[](size_t index)
    {
        return get(index);
    }

    const T& operator[](size_t index) const
    {
        return get(index);
    }

private:
    T* alloc(size_t num)
    {
        return static_cast<T*>(::operator new(num * sizeof(T), std::align_val_t(alignof(T))));
    }

    T* data_ = nullptr;
    size_t size_ = 0;
    size_t numOccupied_ = 0;
    std::vector<uint8_t> occupied_;
};

}
