#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba32f) coherent uniform image2D u_Accumulation;
layout(binding = 1, rgba16f) writeonly uniform image2D u_Output;

uniform sampler2D u_HeightPyramid;
uniform samplerCube u_SkyRadiance;
uniform samplerCube u_SkyIrradiance;
uniform sampler2D u_TerrainSelfShadow;
uniform bool u_HasSkyLight;
uniform bool u_HasTerrainSelfShadow;
uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;
uniform float u_SkyLightIntensity;
uniform vec2 u_TerrainWorldSize;
uniform int u_PyramidLevels;
uniform int u_OutputResolution;
uniform int u_SampleStart;
uniform int u_SamplesThisDispatch;
uniform int u_TargetSamples;
uniform bool u_ResetAccumulation;
uniform float u_HeightBias;

#include "common/heightfield_ray_query.glsl"
#include "common/heightfield_pyramid_sampling.glsl"
#include "common/sampling.glsl"

const float INV_PI = 0.3183098861837907;
const float PI = 3.141592653589793;
const float EPSILON = 0.000001;
const vec3 TERRAIN_ALBEDO = vec3(0.98, 0.96, 0.90);

mat3 BuildTangentFrame(vec3 normal)
{
	vec3 tangent = abs(normal.y) < 0.999
		? normalize(cross(normal, vec3(0.0, 1.0, 0.0)))
		: normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
	vec3 bitangent = normalize(cross(normal, tangent));
	return mat3(tangent, bitangent, normal);
}

vec3 EvaluateTerrainBounceSource(vec2 hitUv, vec3 hitNormal)
{
	vec3 lightDirection = normalize(-u_SunDirection);
	float sunVisibility = 1.0;
	if (u_HasTerrainSelfShadow)
	{
		sunVisibility = texture(u_TerrainSelfShadow, clamp(hitUv, vec2(0.0), vec2(1.0))).r;
	}
	vec3 directIrradiance = u_SunColor * u_SunIntensity *
		max(dot(hitNormal, lightDirection), 0.0) * sunVisibility;

	vec3 skyIrradiance = vec3(0.0);
	if (u_HasSkyLight)
	{
		skyIrradiance = textureLod(u_SkyIrradiance, hitNormal, 0.0).rgb * u_SkyLightIntensity;
	}

	return TERRAIN_ALBEDO * (directIrradiance + skyIrradiance) * INV_PI;
}

void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (outputCoordinate.x >= u_OutputResolution || outputCoordinate.y >= u_OutputResolution) return;

	vec2 uv = (vec2(outputCoordinate) + vec2(0.5)) / float(u_OutputResolution);
	float receiverHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, uv, 0, 2);
	vec3 receiverNormal = TF3D_SamplePyramidTerrainNormal(u_HeightPyramid, uv, u_TerrainWorldSize);
	mat3 tangentFrame = BuildTangentFrame(receiverNormal);

	vec4 accumulation = u_ResetAccumulation
		? vec4(0.0)
		: imageLoad(u_Accumulation, outputCoordinate);
	uint targetSamples = uint(max(u_TargetSamples, 1));

	for (int dispatchSample = 0; dispatchSample < u_SamplesThisDispatch; ++dispatchSample)
	{
		uint sampleIndex = uint(u_SampleStart + dispatchSample);
		vec2 samplePoint = tf3d_sampleHammersley(sampleIndex, targetSamples);
		vec3 localDirection = tf3d_sampleCosineHemisphere(samplePoint.x, samplePoint.y);
		vec3 rayDirection = normalize(tangentFrame * localDirection);
		vec3 incomingRadiance = vec3(0.0);

		if (rayDirection.y > EPSILON)
		{
			vec2 hitUv;
			float hitHeight;
			float hitDistance;
			if (HeightfieldRayIntersect(
				u_HeightPyramid,
				uv,
				receiverHeight + u_HeightBias,
				rayDirection,
				u_TerrainWorldSize,
				u_HeightBias,
				u_PyramidLevels,
				hitUv,
				hitHeight,
				hitDistance))
			{
				vec3 hitNormal = TF3D_SamplePyramidTerrainNormal(
					u_HeightPyramid, hitUv, u_TerrainWorldSize);
				incomingRadiance = EvaluateTerrainBounceSource(hitUv, hitNormal);
			}
		}

		accumulation.rgb += max(incomingRadiance, vec3(0.0)) * PI;
		accumulation.a += 1.0;
	}

	imageStore(u_Accumulation, outputCoordinate, accumulation);
	imageStore(u_Output, outputCoordinate,
		vec4(accumulation.rgb / max(accumulation.a, 1.0),
			accumulation.a / float(targetSamples)));
}
