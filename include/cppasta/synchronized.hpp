#pragma once

#include <mutex>

namespace pasta {

template <typename Data>
class Synchronized
{
public:
    class ConstLockHandle
    {
    public:
        ConstLockHandle(const Synchronized* synchronized)
            : synchronized_(synchronized)
        {
        }

        ~ConstLockHandle()
        {
            if (synchronized_)
            {
                synchronized_->mutex_.unlock();
            }
        }

        // We can only have one Synchronized object (or we might unlock twice).
        // A deep copy (+ lock) might be a sensible thing to do, but you will more likely do this by accident than on purpose.
        ConstLockHandle(const ConstLockHandle&) = delete;
        ConstLockHandle& operator=(const ConstLockHandle&) = delete;

        ConstLockHandle(ConstLockHandle&& other)
            : synchronized_(other.synchronized_)
        {
            other.synchronized_ = nullptr;
        }

        ConstLockHandle& operator=(ConstLockHandle&& other)
        {
            synchronized_ = other.synchronized_;
            other.synchronized_ = nullptr;
        }

        const Data* operator->() const
        {
            return &synchronized_->data_;
        }

        const Data& operator*() const
        {
            return synchronized_->data_;
        }

        /*
         * This is a little helper that invites non-buggy usage in loops.
         * You might be tempted to do something like: `for (const auto& val : *sync.lock())`,
         * but this way the range object is of type `const Data&` and the lock handle will have been destroyed
         * in the first iteration!
         * If the lock handle provides begin/end, you can do this: `for (const auto& val : sync.lock())`
         * and the range object will be the lock handle, consequently being kept alive until the end of the loop.
         */
        auto begin() const
        {
            return synchronized_->data_.begin();
        }

        auto end() const
        {
            return synchronized_->data_.end();
        }

    private:
        const Synchronized* synchronized_;
    };

    class LockHandle
    {
    public:
        LockHandle(Synchronized* synchronized)
            : synchronized_(synchronized)
        {
        }

        ~LockHandle()
        {
            if (synchronized_)
            {
                synchronized_->mutex_.unlock();
            }
        }

        LockHandle(const LockHandle&) = delete;
        LockHandle& operator=(const LockHandle&) = delete;

        LockHandle(LockHandle&& other)
            : synchronized_(other.synchronized_)
        {
            other.synchronized_ = nullptr;
        }

        LockHandle& operator=(LockHandle&& other)
        {
            synchronized_ = other.synchronized_;
            other.synchronized_ = nullptr;
        }

        const Data* operator->() const
        {
            return &synchronized_->data_;
        }

        Data* operator->()
        {
            return &synchronized_->data_;
        }

        const Data& operator*() const
        {
            return synchronized_->data_;
        }

        Data& operator*()
        {
            return synchronized_->data_;
        }

        auto begin() const
        {
            return synchronized_->data_.begin();
        }

        auto end() const
        {
            return synchronized_->data_.end();
        }

        auto begin()
        {
            return synchronized_->data_.begin();
        }

        auto end()
        {
            return synchronized_->data_.end();
        }

    private:
        Synchronized* synchronized_;
    };

    template <typename... Args>
    Synchronized(Args&&... args)
        : data_(std::forward<Args>(args)...)
    {
    }

    // Considering the wrapped value needs to be synchronized, we can't just copy or move the data
    // whenever we want, but likely we have to lock it.
    // The specifics are likely different enough so that we cannot do anything sensible in the default case.
    Synchronized(const Synchronized&) = delete;
    Synchronized(Synchronized&&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;
    Synchronized& operator=(Synchronized&&) = delete;

    LockHandle lock()
    {
        mutex_.lock();
        return LockHandle(this);
    }

    ConstLockHandle lock() const
    {
        mutex_.lock();
        return ConstLockHandle(this);
    }

    ConstLockHandle lockConst() const
    {
        mutex_.lock();
        return ConstLockHandle(this);
    }

private:
    mutable std::mutex mutex_;
    Data data_;
};

}
