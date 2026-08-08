#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, rgba32f) writeonly uniform image2D u_Result;

uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform vec3 u_RayOrigin;
uniform vec3 u_RayDirection;
uniform vec2 u_TerrainMinimumXZ;
uniform vec2 u_TerrainWorldSize;
uniform float u_TerrainHeightOffset;
uniform float u_HeightBias;

#include "common/heightfield_ray_query.glsl"

void main()
{

	vec4 result = vec4(0.0);
	if (u_PyramidLevels > 0 &&
		all(greaterThan(u_TerrainWorldSize, vec2(HEIGHTFIELD_QUERY_EPSILON))))
	{
		vec2 hitUv;
		float hitHeight;
		float hitDistance;
		if (HeightfieldWorldRayIntersect(
			u_HeightPyramid,
			u_RayOrigin,
			u_RayDirection,
			u_TerrainMinimumXZ,
			u_TerrainWorldSize,
			u_TerrainHeightOffset,
			u_HeightBias,
			u_PyramidLevels,
			hitUv,
			hitHeight,
			hitDistance))
		{
			vec3 hitPosition = u_RayOrigin + u_RayDirection * hitDistance;
			hitPosition.y = hitHeight + u_TerrainHeightOffset;
			result = vec4(hitPosition, 1.0);
		}
	}

	imageStore(u_Result, ivec2(0), result);
}
