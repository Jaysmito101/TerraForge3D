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
	DEFINE_UNIFORM(vec4, _Resolution)
END_UNIFORMS()

struct TerrainPointData
{
	vec4 a;
	vec4 b;
	vec4 c;
	vec4 d;
};



BEGIN_SSB(0, MeshData)
	TerrainPointData pointData[];	
END_SSB()



void main()
{
	// DEBUG_PRINT("Hello World!");
	int index = 0;
	GET_SSB_INDEX(texCoord, index);

	vec3 newPosition = position.xyz + normalize(-normal.xyz) * pointData[index].a.x;
	gl_Position = UNIFORM(_PV) * UNIFORM(_Model) * vec4(newPosition, 1.0f);
	Position = newPosition;
	Normal = -normal.xyz;
}
