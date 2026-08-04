#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout (std430, binding = 0) buffer DataSourceBuffer
{
	float dataSource[];
};

layout (std430, binding = 1) buffer DataTargetBuffer
{
	float dataTarget[];
};

// uniforms
uniform int u_Resolution;
uniform int u_Seed;
uniform bool u_UseSeedTexture;
uniform sampler2D u_SeedTexture;
uniform int u_NoiseAlgorithm;
uniform float u_NoiseScale;
uniform float u_NoiseSeed;
uniform int u_NoiseOctaves;
uniform float u_NoiseWarp;
uniform float u_NoiseJitter;
uniform float u_Strength;
uniform float u_Influence;
uniform float u_Frequency;
uniform float u_Lacunarity;
uniform float u_Persistence;
uniform int u_MixMethod;
uniform int u_TransformFactor;
uniform vec3 u_Offset;
uniform float u_NoiseOctaveStrengths[16];
uniform int u_NoiseOctaveStrengthsCount;
uniform int u_SlopeSmoothingRadius;
uniform vec2 u_TransformRange;
uniform float u_SlopeSamplingRadius;
uniform bool m_UseGaussianPreFilter;

#include "common/noise_2d.glsl"
#include "common/noise_3d.glsl"
#include "common/base_shape_helpers.glsl"

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

float gaussianSample(ivec2 offset)
{
	const float filterMask[5][5] = {
		{ 0.000229, 0.005977, 0.060598, 0.005977, 0.000229 },
		{ 0.005977, 0.156150, 1.579180, 0.156150, 0.005977 },
		{ 0.060598, 1.579180, 15.961800, 1.579180, 0.060598 },
		{ 0.005977, 0.156150, 1.579180, 0.156150, 0.005977 },
		{ 0.000229, 0.005977, 0.060598, 0.005977, 0.000229 }
	};

	float sum = 0.0;
	float weightSum = 0.0;
	
	for (int i = -2; i <= 2; i++)
	{
		for (int j = -2; j <= 2; j++)
		{
			ivec2 offsetiv2 = offset + ivec2(i, j);
			if (offsetiv2.x < 0 || offsetiv2.x >= u_Resolution || offsetiv2.y < 0 || offsetiv2.y >= u_Resolution) continue;
			float weight = filterMask[i + 2][j + 2];
			sum += dataSource[PixelCoordToDataOffset(uint(offsetiv2.x), uint(offsetiv2.y))] * weight;
			weightSum += weight;
		}
	}

	return sum / max(weightSum, 0.000001f);
}

float slopeHeightAt(ivec2 coord)
{
	coord = clamp(coord, ivec2(0), ivec2(u_Resolution - 1));
	if (m_UseGaussianPreFilter) return gaussianSample(coord);
	return dataSource[PixelCoordToDataOffset(uint(coord.x), uint(coord.y))];
}

float calculateSlopeFactorAtCoord(ivec2 offsetb, ivec2 offsetc, float radius)
{
	if (u_Resolution < 2) return 0.0f;

	ivec2 center = clamp(offsetb + offsetc, ivec2(0), ivec2(u_Resolution - 1));
	int stepPixels = max(int(round(abs(radius))), 1);
	float step = float(stepPixels);

	float dX =
		(3.0f * slopeHeightAt(center + ivec2( stepPixels, -stepPixels)) +
		 10.0f * slopeHeightAt(center + ivec2( stepPixels,  0)) +
		 3.0f * slopeHeightAt(center + ivec2( stepPixels,  stepPixels)) -
		 3.0f * slopeHeightAt(center + ivec2(-stepPixels, -stepPixels)) -
		10.0f * slopeHeightAt(center + ivec2(-stepPixels,  0)) -
		 3.0f * slopeHeightAt(center + ivec2(-stepPixels,  stepPixels))) / (32.0f * step);
	float dY =
		(3.0f * slopeHeightAt(center + ivec2(-stepPixels,  stepPixels)) +
		10.0f * slopeHeightAt(center + ivec2( 0,  stepPixels)) +
		 3.0f * slopeHeightAt(center + ivec2( stepPixels,  stepPixels)) -
		 3.0f * slopeHeightAt(center + ivec2(-stepPixels, -stepPixels)) -
		10.0f * slopeHeightAt(center + ivec2( 0, -stepPixels)) -
		 3.0f * slopeHeightAt(center + ivec2( stepPixels, -stepPixels))) / (32.0f * step);

	return length(vec2(dX, dY)) * float(u_Resolution);
}

float calculateSlopeFactor()
{
	ivec2 offsetv2 = ivec2(gl_GlobalInvocationID.xy);
	int smoothingRadius = clamp(u_SlopeSmoothingRadius, 0, 20);
	int samplingRadius = max(int(round(abs(u_SlopeSamplingRadius))), 1);

	float factor = 0.0f;
	float weightSum = 0.0f;
	float sigma = max(float(smoothingRadius) * 0.5f, 1.0f);

	for (int i = -smoothingRadius; i <= smoothingRadius; i++)
	{
		for (int j = -smoothingRadius; j <= smoothingRadius; j++)
		{
			ivec2 sampleOffset = ivec2(i, j) * samplingRadius;
			float weight = exp(-0.5f * (float(i * i + j * j) / (sigma * sigma)));
			factor += calculateSlopeFactorAtCoord(offsetv2, sampleOffset, float(samplingRadius)) * weight;
			weightSum += weight;
		}
	}

	factor /= max(weightSum, 0.000001f);

	vec2 transformRange = vec2(min(u_TransformRange.x, u_TransformRange.y), max(u_TransformRange.x, u_TransformRange.y));
	factor = tf3d_shape_smoothstep(transformRange.x, transformRange.y, factor);

	return factor;
}


void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	if (offsetv2.x >= uint(u_Resolution) || offsetv2.y >= uint(u_Resolution)) return;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);
	vec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);
	if (u_UseSeedTexture)
	{
		seed = texture(u_SeedTexture, uv).rgb; 
	}
	float frequencyInput = clamp(abs(u_Frequency), 0.001f, 16.0f);
	float lacunarity = clamp(u_Lacunarity, 1.0f, 4.0f);
	float persistence = clamp(u_Persistence, 0.0f, 1.0f);
	vec3 offsetInput = clamp(u_Offset, vec3(-10000.0f), vec3(10000.0f));
	seed = seed * frequencyInput + offsetInput + vec3(u_Seed % 100);

	vec2 noiseDomain = seed.xy;
	float safeWarp = clamp(abs(u_NoiseWarp), 0.0f, 4.0f);
	if (safeWarp > 0.0001f)
	{
		noiseDomain += vec2(
			tf3d_noise2(noiseDomain * 0.5f + vec2(17.0f, 5.0f), u_NoiseAlgorithm, u_NoiseJitter, float(u_Seed) + 13.0f),
			tf3d_noise2(noiseDomain * 0.5f + vec2(-7.0f, 23.0f), u_NoiseAlgorithm, u_NoiseJitter, float(u_Seed) + 37.0f)) * safeWarp;
	}

	float n = 0.0f;
	float amplitude = 1.0f;
	float amplitudeSum = 0.0f;
	int octaveCount = clamp(min(u_NoiseOctaves, u_NoiseOctaveStrengthsCount), 0, 16);
	for (int i = 0; i < octaveCount; ++i)
	{
		float octaveStrength = clamp(u_NoiseOctaveStrengths[i], 0.0f, 1.0f);
		n += tf3d_noise2(noiseDomain, u_NoiseAlgorithm, u_NoiseJitter, float(u_Seed) + float(i) * 11.73f)
			* amplitude * octaveStrength;
		amplitudeSum += amplitude * octaveStrength;
		noiseDomain = noiseDomain * lacunarity + vec2(17.13f, 9.71f);
		amplitude *= persistence;
	}
	n /= max(amplitudeSum, 0.0001f);


	if ( u_TransformFactor == 1) n = n * calculateSlopeFactor();
	else if ( u_TransformFactor == 2)
	{
		vec2 transformRange = vec2(min(u_TransformRange.x, u_TransformRange.y), max(u_TransformRange.x, u_TransformRange.y));
		n = n * tf3d_shape_smoothstep(transformRange.x, transformRange.y, dataSource[offset]);
	}

	n = n * clamp(u_Strength, -4.0f, 4.0f) * clamp(u_Influence, 0.0f, 1.0f);

	if ( u_MixMethod == 0 ) dataTarget[offset] = dataSource[offset] + n;
	else if ( u_MixMethod == 1 ) dataTarget[offset] = dataSource[offset] * n;
	else if ( u_MixMethod == 2 ) dataTarget[offset] = dataSource[offset] * n + dataSource[offset];
	else if ( u_MixMethod == 3 ) dataTarget[offset] = n;
	else dataTarget[offset] = dataSource[offset];

}
