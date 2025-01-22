#pragma once

#if PLATFORM_DESKTOP
    #include <gl3w/GL/gl3w.h>
    #include <gl3w/GL/glcorearb.h>
#elif PLATFORM_WEB
    #include <emscripten.h>
    #include <SDL_opengl.h>
    #include <SDL_opengl_glext.h>
#endif