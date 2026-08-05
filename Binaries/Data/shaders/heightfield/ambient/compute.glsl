#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, rgba16f) writeonly uniform image2D u_Output;

uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform int u_OutputResolution;
uniform vec2 u_TerrainWorldSize;
uniform float u_AoRadius;
uniform float u_HeightBias;

#include "common/heightfield_pyramid_sampling.glsl"

const int HORIZON_AZIMUTH_COUNT = 16;
const int SKY_ELEVATION_SAMPLES = 4;
const float PI = 3.141592653589793;
const float HALF_PI = 1.5707963267948966;
const float EPSILON = 0.000001;

float ComputeHorizonAngle(vec2 uv, vec2 directionUv, float radiusWorld)
{
	ivec2 pyramidSize = textureSize(u_HeightPyramid, 0);
	float baseSampleStepUv = 1.0 / float(max(pyramidSize.x - 1, 1));
	float baseSampleStepWorld = u_TerrainWorldSize.x * baseSampleStepUv;
	float horizonTangent = 0.0;

	for (int level = 0; level < 32; ++level)
	{
		if (level >= u_PyramidLevels) break;
		float levelScale = exp2(float(level));
		float distanceWorld = baseSampleStepWorld * levelScale;
		if (distanceWorld > radiusWorld + baseSampleStepWorld * 0.5) break;

		vec2 sampleUv = uv + directionUv * distanceWorld;
		if (any(lessThan(sampleUv, vec2(0.0))) || any(greaterThan(sampleUv, vec2(1.0)))) break;

		float receiverHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, uv, level, 2);
		float casterHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, sampleUv, level, 2);
		float heightDifference = casterHeight - receiverHeight - u_HeightBias;
		horizonTangent = max(horizonTangent, heightDifference / max(distanceWorld, EPSILON));
	}

	return clamp(atan(max(horizonTangent, 0.0)), 0.0, HALF_PI);
}

vec3 SkyDirection(vec2 horizontalDirection, float elevation)
{
	float horizontalLength = cos(elevation);
	return normalize(vec3(horizontalDirection.x * horizontalLength,
		sin(elevation), horizontalDirection.y * horizontalLength));
}

// based on: https://cim.mcgill.ca/~derek/files/publication-files/hfvisib.pdf
void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (outputCoordinate.x >= u_OutputResolution || outputCoordinate.y >= u_OutputResolution) return;

	vec2 uv = (vec2(outputCoordinate) + vec2(0.5)) / float(u_OutputResolution);
	vec3 terrainNormal = TF3D_SamplePyramidTerrainNormal(u_HeightPyramid, uv, u_TerrainWorldSize);
	ivec2 pyramidSize = textureSize(u_HeightPyramid, 0);
	float baseSampleStepWorld = u_TerrainWorldSize.x /
		float(max(pyramidSize.x - 1, 1));
	float radiusWorld = max(u_AoRadius, baseSampleStepWorld);

	float visibleWeight = 0.0;
	float unoccludedWeight = 0.0;
	vec3 visibleDirectionSum = vec3(0.0);
	vec3 unoccludedDirectionSum = vec3(0.0);
	float azimuthStep = 2.0 * PI / float(HORIZON_AZIMUTH_COUNT);
	float horizonAngles[HORIZON_AZIMUTH_COUNT];

	for (int azimuthIndex = 0; azimuthIndex < HORIZON_AZIMUTH_COUNT; ++azimuthIndex)
	{
		float azimuth = float(azimuthIndex) * azimuthStep;
		vec2 horizontalDirection = vec2(cos(azimuth), sin(azimuth));
		vec2 directionUv = vec2(horizontalDirection.x, -horizontalDirection.y) / u_TerrainWorldSize;
		horizonAngles[azimuthIndex] = ComputeHorizonAngle(uv, directionUv, radiusWorld);
	}

	for (int azimuthIndex = 0; azimuthIndex < HORIZON_AZIMUTH_COUNT; ++azimuthIndex)
	{
		int nextAzimuthIndex = (azimuthIndex + 1) % HORIZON_AZIMUTH_COUNT;
		float azimuth = (float(azimuthIndex) + 0.5) * azimuthStep;
		vec2 horizontalDirection = vec2(cos(azimuth), sin(azimuth));
		float horizonAngle = 0.5 * (horizonAngles[azimuthIndex] + horizonAngles[nextAzimuthIndex]);
		float visibleElevationSpan = HALF_PI - horizonAngle;
		float visibleElevationStep = visibleElevationSpan / float(SKY_ELEVATION_SAMPLES);
		float fullElevationStep = HALF_PI / float(SKY_ELEVATION_SAMPLES);

		for (int elevationIndex = 0; elevationIndex < SKY_ELEVATION_SAMPLES; ++elevationIndex)
		{
			float fullElevation = (float(elevationIndex) + 0.5) * fullElevationStep;
			vec3 fullDirection = SkyDirection(horizontalDirection, fullElevation);
			float fullWeight = max(dot(terrainNormal, fullDirection), 0.0) *
				cos(fullElevation) * fullElevationStep * azimuthStep;
			unoccludedWeight += fullWeight;
			unoccludedDirectionSum += fullDirection * fullWeight;

			if (visibleElevationSpan <= EPSILON) continue;
			float visibleElevation = horizonAngle +
				(float(elevationIndex) + 0.5) * visibleElevationStep;
			vec3 visibleDirection = SkyDirection(horizontalDirection, visibleElevation);
			float visibleSampleWeight = max(dot(terrainNormal, visibleDirection), 0.0) *
				cos(visibleElevation) * visibleElevationStep * azimuthStep;
			visibleWeight += visibleSampleWeight;
			visibleDirectionSum += visibleDirection * visibleSampleWeight;
		}
	}

	float ambientVisibility = visibleWeight / max(unoccludedWeight, EPSILON);
	ambientVisibility = clamp(ambientVisibility, 0.0, 1.0);

	vec3 fullMeanDirection = unoccludedDirectionSum / max(unoccludedWeight, EPSILON);
	vec3 visibleMeanDirection = visibleDirectionSum / max(visibleWeight, EPSILON);
	vec3 bentNormal = terrainNormal;
	if (visibleWeight > EPSILON)
	{
		bentNormal = normalize(terrainNormal + (visibleMeanDirection - fullMeanDirection));
	}

	imageStore(u_Output, outputCoordinate,
		vec4(ambientVisibility, TF3D_OctahedralEncode(bentNormal), 1.0));
}
