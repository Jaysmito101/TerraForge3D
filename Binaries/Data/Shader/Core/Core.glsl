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

#define BEGIN_SSB(x, n) layout (std430, binding = x) buffer n {
#define END_SSB() };

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

#define BEGIN_SSB(x, n) layout (std140, binding = x) buffer n {
#define END_SSB() };

#define GET_MOUSE_PICK_ID() UNIFORM(_Engine).x;

#endif // TF3D_VULKAN

// Common macros

#define GET_SSB_INDEX(tc, out) \
	{ \
		int rX_ssb_index = int(tc.x * (UNIFORM(_Resolution).x - 1)); \
		int rY_ssb_index = int(tc.y * (UNIFORM(_Resolution).y - 1)); \
		out = rY_ssb_index * int(UNIFORM(_Resolution).x) + rX_ssb_index ; \
	}

#endif // CORE_GLSL