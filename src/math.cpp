#include "cppasta/math.hpp"

#include <algorithm>
#include <cassert>

namespace pasta {

float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float unlerp(float val, float a, float b)
{
    const auto clamped = std::clamp(val, std::min(a, b), std::max(a, b));
    return (clamped - a) / (b - a);
}

float rescale(float val, float fromA, float fromB, float toA, float toB)
{
    return lerp(toA, toB, unlerp(val, fromA, fromB));
}

float approach(float current, float target, float delta)
{
    assert(delta > 0.0f);
    const auto diff = target - current;
    if (diff > 0.0f) {
        current += std::min(diff, delta);
    } else if (diff < 0.0f) {
        current -= std::min(-diff, delta);
    }
    return current;
}

}
