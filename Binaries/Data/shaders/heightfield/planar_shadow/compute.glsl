#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, r8) writeonly uniform image2D u_Output;

uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform int u_OutputResolution;
uniform vec3 u_LightDirection;
uniform vec2 u_AtlasMinimumXZ;
uniform vec2 u_AtlasWorldSize;
uniform vec2 u_TerrainMinimumXZ;
uniform vec2 u_TerrainWorldSize;
uniform float u_TerrainHeightOffset;
uniform float u_ReceiverHeight;
uniform float u_ReceiverBias;
uniform float u_MaxDistance;

#include "common/heightfield_ray_query.glsl"

void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (outputCoordinate.x >= u_OutputResolution || outputCoordinate.y >= u_OutputResolution) return;

	vec2 atlasUv = (vec2(outputCoordinate) + vec2(0.5)) / float(u_OutputResolution);
	vec2 worldXZ = u_AtlasMinimumXZ + atlasUv * u_AtlasWorldSize;
	vec2 terrainMaximumXZ = u_TerrainMinimumXZ + u_TerrainWorldSize;
	// The generated plane's UV-Y axis points toward -world-Z.
	vec2 terrainUv = vec2(
		(worldXZ.x - u_TerrainMinimumXZ.x) / u_TerrainWorldSize.x,
		(terrainMaximumXZ.y - worldXZ.y) / u_TerrainWorldSize.y);
	bool occluded = HeightfieldRayOccludedFromPlane(
		u_HeightPyramid,
		terrainUv,
		u_ReceiverHeight + u_ReceiverBias,
		u_LightDirection,
		u_TerrainWorldSize,
		u_TerrainHeightOffset,
		u_ReceiverBias,
		u_MaxDistance,
		u_PyramidLevels);

	imageStore(u_Output, outputCoordinate, vec4(occluded ? 0.0 : 1.0));
}
