#pragma once

#include "tools/gui/GLhelper.h"
#include "tools/gui/geometry/Vec2.h"
#include "tools/core/types.h"

namespace tools
{
    struct Texture  //  Simple data structure to store OpenGL texture information
    {
        Texture(): Texture({}, 0, 0)
        {}

        Texture(std::vector<unsigned char> _buffer, int _width, int _height, GLuint _image = 0)
            : buffer(std::move(_buffer))
            , gl_handler(_image)
            , width(_width)
            , height(_height)
        {}

        GLuint gl_handler;                 // OpenGL handler to the texture in the GPU
        std::vector<unsigned char> buffer; // Image buffer loaded into memory
        u32_t width;                       // in pixels
        u32_t height;                      // in pixels

        Vec2 size() const
        { return { (float)width, (float)height }; }

        u64_t id() const
        { return gl_handler; }
    };
}
