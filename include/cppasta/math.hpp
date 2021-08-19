#pragma once

namespace pasta {

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

float lerp(float a, float b, float t);
float unlerp(float val, float a, float b);
float rescale(float val, float fromA, float fromB, float toA, float toB);
float approach(float current, float target, float delta);

}
