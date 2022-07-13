#version 430 core

#include "Core/Extensions.glsl"
#include "Core/Core.glsl"

#include "Temp.glsl"

DEFINE_VERTEX_INPUT(0, vec4, position)
DEFINE_VERTEX_INPUT(1, vec4, texCoord)
DEFINE_VERTEX_INPUT(2, vec4, normal)
DEFINE_VERTEX_INPUT(3, vec4, extra)

DEFINE_VERTEX_OUTPUT(0, vec3, Position)
DEFINE_VERTEX_OUTPUT(1, vec3, Normal)

BEGIN_UNIFORMS()
	DEFINE_UNIFORM(ivec4, _Engine)
	DEFINE_UNIFORM(mat4, _Model)
	DEFINE_UNIFORM(mat4, _PV)
END_UNIFORMS()

void main()
{
	// DEBUG_PRINT("Hello World!");
	gl_Position = UNIFORM(_PV) * UNIFORM(_Model) * vec4(position.xyz + vec3(0.0f, noise(position.xyz), 0.0f), 1.0f);
	Position = position.xyz;
	Normal = normal.xyz;
}
