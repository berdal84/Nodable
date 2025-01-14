#pragma once

#if PLATFORM_DESKTOP
#include <gl3w/GL/gl3w.h>
#include <gl3w/GL/glcorearb.h>
#elif PLATFORM_WEB
#include <GLES3/gl3.h>
#endif