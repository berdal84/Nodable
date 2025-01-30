#pragma once
#include <glm/common.hpp>

namespace tools
{
    static float normalize(float _value, float _min, float _max)
    { return glm::clamp(_value, _min, _max) / (_max - _min); }

    static float clamped_lerp(float a, float b, float f)
    { return glm::mix(a, b, glm::clamp( f, 0.f, 1.f) ); }

    static double clamped_lerp(double a, double b, double f)
    { return glm::mix(a, b, glm::clamp( f, 0.0, 1.0) ); }

    static float wave(float min, float max, double time, float speed)
    {
        float factor = 0.5f * ( 1.0f + std::sin( time * speed ) );
        return clamped_lerp(min, max, factor);
    }

    static i64_t signed_diff(u64_t a, u64_t b)
    {
        if ( a == b )
            return 0;

        i64_t sign =  a > b ? 1 : -1;
        u64_t unsigned_diff = sign > 0 ? a - b : b - a;

        ASSERT(unsigned_diff <= std::numeric_limits<i64_t>::max() ); // capacity check
        ASSERT(unsigned_diff <= std::numeric_limits<i64_t>::min() ); // capacity check

        return sign * (i64_t)unsigned_diff;
    }
}
