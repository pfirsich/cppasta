#pragma once

#include <glm/glm.hpp>

namespace pasta {

template <typename T>
T safeNormalize(const T& vec)
{
    const auto len = glm::length(vec) + 1e-5f;
    return vec / len;
}

}
