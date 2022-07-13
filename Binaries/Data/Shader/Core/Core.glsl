#ifndef CORE_GLSL


#ifdef TF3D_OPENGL

#define DEFINE_VERTEX_INPUT(loc, type, name) layout (location = loc) in type name;
#define DEFINE_VERTEX_OUTPUT(loc, type, name) out type name;

#define DEFINE_FRAGMENT_INPUT(loc, type, name) in type name;
#define DEFINE_FRAGMENT_OUTPUT(loc, type, name) layout (location = loc) out type name;

#define BEGIN_UNIFORMS()
#define END_UNIFORMS()
#define DEFINE_UNIFORM(type, name) uniform type name = type(1.0f);
#define UNIFORM(name) name

#define GET_MOUSE_PICK_ID() _MousePickID;

#elif defined(TF3D_VULKAN)

#define DEFINE_VERTEX_INPUT(loc, type, name) layout (location = loc) in type name;
#define DEFINE_VERTEX_OUTPUT(loc, type, name) layout (location = loc) out type name;

#define DEFINE_FRAGMENT_INPUT(loc, type, name) layout (location = loc) in type name;
#define DEFINE_FRAGMENT_OUTPUT(loc, type, name) layout (location = loc) out type name;

#define BEGIN_UNIFORMS() layout (push_constant) uniform Constants {
#define END_UNIFORMS() } PushConstants;
#define DEFINE_UNIFORM(type, name)     type name;
#define UNIFORM(name) PushConstants.name

#define GET_MOUSE_PICK_ID() UNIFORM(_Engine).x;

#endif // TF3D_VULKAN

#endif // CORE_GLSL