#pragma once

namespace pasta {

/*
 * Usage Example:
 * class Foo : public Singleton<Foo> {
 *     friend class Singleton<Foo>;
 * public:
 *     // ...
 * private:
 *      Foo();
 * };
 */

template <typename Derived>
class Singleton {
public:
    static Derived& instance()
    {
        static Derived inst;
        return inst;
    }

protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

}
