#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba16f) writeonly uniform image2D u_Output;

uniform sampler2D u_HeightPyramid;
uniform sampler2D u_RawGI;
uniform int u_OutputResolution;
uniform vec2 u_TerrainWorldSize;
uniform float u_HeightSigma;
uniform float u_NormalPower;

#include "common/heightfield_pyramid_sampling.glsl"

const float EPSILON = 0.000001;

void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (outputCoordinate.x >= u_OutputResolution || outputCoordinate.y >= u_OutputResolution) return;

	ivec2 centerCoordinate = clamp(outputCoordinate,
		ivec2(0), ivec2(u_OutputResolution - 1));
	vec2 centerUv = (vec2(centerCoordinate) + vec2(0.5)) / float(u_OutputResolution);
	float centerHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, centerUv, 0, 2);
	vec3 centerNormal = TF3D_SamplePyramidTerrainNormal(
		u_HeightPyramid, centerUv, u_TerrainWorldSize);

	vec3 filteredRadiance = vec3(0.0);
	float totalWeight = 0.0;
	float centerProgress = texelFetch(u_RawGI, centerCoordinate, 0).a;
	for (int y = -2; y <= 2; ++y)
	{
		for (int x = -2; x <= 2; ++x)
		{
			ivec2 sampleCoordinate = clamp(outputCoordinate + ivec2(x, y),
				ivec2(0), ivec2(u_OutputResolution - 1));
			vec2 sampleUv = (vec2(sampleCoordinate) + vec2(0.5)) /
				float(u_OutputResolution);
			float sampleHeight = TF3D_SamplePyramidChannel(
				u_HeightPyramid, sampleUv, 0, 2);
			vec3 sampleNormal = TF3D_SamplePyramidTerrainNormal(
				u_HeightPyramid, sampleUv, u_TerrainWorldSize);
			vec2 offset = vec2(ivec2(x, y));
			float spatialWeight = exp(-0.35 * dot(offset, offset));
			float heightWeight = exp(-abs(sampleHeight - centerHeight) /
				max(u_HeightSigma, EPSILON));
			float normalWeight = pow(max(dot(centerNormal, sampleNormal), 0.0),
				max(u_NormalPower, 0.0));
			float weight = spatialWeight * heightWeight * normalWeight;
			filteredRadiance += texelFetch(u_RawGI, sampleCoordinate, 0).rgb * weight;
			totalWeight += weight;
		}
	}

	imageStore(u_Output, outputCoordinate, vec4(
		filteredRadiance / max(totalWeight, EPSILON), centerProgress));
}
