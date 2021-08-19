#include "cppasta/random.hpp"

namespace pasta {

std::default_random_engine& getRng()
{
    static std::default_random_engine rng { std::random_device {}() };
    return rng;
}
}
