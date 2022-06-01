#ifndef CORE_GLSL

#ifdef TF3D_OPENGL

#define GET_ENGINE_DATA(name) name

#elif defined(TF3D_VULKAN)

#define GET_ENGINE_DATA(name) PushConstants.name

#endif

#endif // CORE_GLSL