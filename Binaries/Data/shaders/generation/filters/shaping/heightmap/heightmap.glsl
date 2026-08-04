#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

layout(binding = 4) uniform sampler2D u_Heightmap;

uniform bool u_HasHeightmap;
uniform float u_HeightScale;
uniform float u_HeightOffset;
uniform float u_MapScale;
uniform vec2 u_MapPosition;
uniform float u_MapRotation;
uniform float u_EdgeFeather;
uniform bool u_Invert;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	if (!u_HasHeightmap)
	{
		outputData[indexOf(coordinate)] = 1.0e20f;
		return;
	}

	vec2 terrainUv = (vec2(coordinate) + vec2(0.5f)) / float(u_Resolution);
	terrainUv.y = 1.0f - terrainUv.y;
	vec2 centered = terrainUv - vec2(0.5f) - u_MapPosition;
	float mapScale = max(abs(u_MapScale), 0.0001f);
	float angle = radians(u_MapRotation);
	mat2 inverseRotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
	vec2 mapUv = inverseRotation * centered / mapScale + vec2(0.5f);

	float edgeDistance = min(min(mapUv.x, 1.0f - mapUv.x), min(mapUv.y, 1.0f - mapUv.y));
	float feather = clamp(u_EdgeFeather, 0.0f, 0.5f);
	float coverage = feather > 0.0001f ? smoothstep(0.0f, feather, edgeDistance) : step(0.0f, edgeDistance);
	if (coverage <= 0.0001f)
	{
		outputData[indexOf(coordinate)] = 1.0e20f;
		return;
	}

	float height = textureLod(u_Heightmap, clamp(mapUv, vec2(0.0f), vec2(1.0f)), 0.0f).r;
	if (u_Invert) height = 1.0f - height;

	float importedHeight = height * u_HeightScale + u_HeightOffset;
	outputData[indexOf(coordinate)] = mix(sampleInput(coordinate), importedHeight, coverage);
}
