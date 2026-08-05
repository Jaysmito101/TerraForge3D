#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, r8) writeonly uniform image2D u_Output;

uniform sampler2D u_Heightmap;
uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform int u_OutputResolution;
uniform vec3 u_LightDirection;
uniform vec2 u_TerrainWorldSize;
uniform float u_HeightBias;

#include "common/heightfield_ray_query.glsl"

void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (outputCoordinate.x >= u_OutputResolution || outputCoordinate.y >= u_OutputResolution) return;

	vec2 uv = (vec2(outputCoordinate) + vec2(0.5)) / float(u_OutputResolution);
	ivec2 heightmapSize = textureSize(u_Heightmap, 0);
	ivec2 heightCoordinate = clamp(ivec2(uv * vec2(heightmapSize)), ivec2(0), heightmapSize - 1);
	float height = texelFetch(u_Heightmap, heightCoordinate, 0).r;
	bool occluded = HeightfieldRayOccluded(
		u_HeightPyramid,
		uv,
		height + u_HeightBias,
		u_LightDirection,
		u_TerrainWorldSize,
		u_HeightBias,
		u_PyramidLevels);

	imageStore(u_Output, outputCoordinate, vec4(occluded ? 0.0 : 1.0));
}
