#pragma once
#include "glm/glm/common.hpp"
#include "glm/glm/vec4.hpp"

namespace tools
{
    struct Vec4
    {
        float x, y, z, w;

        constexpr Vec4()
        : Vec4(0.f, 0.f, 0.f, 0.f)
        {};
        constexpr Vec4(float x, float y, float z, float w = 0.f)
        : x(x), y(y), z(z), w(w)
        {}

        constexpr Vec4(const glm::vec4& vec)
        : Vec4(vec.x, vec.y, vec.z, vec.w)
        {}

        operator glm::vec4() const
        { return {x, y, z, w}; }

        static Vec4 lerp(Vec4 _source, Vec4 _target, float _factor)
        { return glm::mix((glm::vec4)_source, (glm::vec4)_target, glm::clamp(_factor, 0.0f, 1.0f)); }
    };
}
