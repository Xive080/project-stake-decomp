// Cabecera GL multiplataforma: GL 3.3 core (escritorio) o GLES 3.0 (Android). Sin rutas absolutas ni dependencias externas.
#pragma once
#if defined(__APPLE__)
  #include <OpenGL/gl3.h>
  #define VN_GL_ES 0
#elif defined(__ANDROID__)
  #include <GLES3/gl3.h>
  #define VN_GL_ES 1
#else
  #include <SDL3/SDL_opengl.h>
  #include <SDL3/SDL_video.h>
  #define VN_GL_ES 0
  #define VN_NEED_LOADER 1
#endif
namespace vn { bool loadGL(); }
