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

#include "common/noise_3d.glsl"

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

float gaussianSample(uvec2 offset)
{
	const float filterMask[5][5] = {
		{ 0.000229, 0.005977, 0.060598, 0.005977, 0.000229 },
		{ 0.005977, 0.156150, 1.579180, 0.156150, 0.005977 },
		{ 0.060598, 1.579180, 15.961800, 1.579180, 0.060598 },
		{ 0.005977, 0.156150, 1.579180, 0.156150, 0.005977 },
		{ 0.000229, 0.005977, 0.060598, 0.005977, 0.000229 }
	};

	float sum = 0.0;
	
	for (int i = -2; i <= 2; i++)
	{
		for (int j = -2; j <= 2; j++)
		{
			ivec2 offsetiv2 = ivec2(offset.x + i, offset.y + j);
			if (offsetiv2.x < 0 || offsetiv2.x >= u_Resolution || offsetiv2.y < 0 || offsetiv2.y >= u_Resolution) continue;
			sum += dataSource[PixelCoordToDataOffset(offsetiv2.x, offsetiv2.y)] * filterMask[i + 2][j + 2];
		}
	}

	return sum;
}

float calculateSlopeFactorAtCoord(uvec2 offsetb, uvec2 offsetc, float radius)
{
	uvec2 offsetv2 = offsetb + offsetc;
	
	if (offsetv2.x == 0) offsetv2.x = 1;
	if (offsetv2.y == 0) offsetv2.y = 1;
	if (offsetv2.x == u_Resolution - 1) offsetv2.x = u_Resolution - 2;
	if (offsetv2.y == u_Resolution - 1) offsetv2.y = u_Resolution - 2;
	

	float T = 0.0f, B = 0.0f, L = 0.0f, R = 0.0f;

	if (m_UseGaussianPreFilter)
	{
		T = gaussianSample(offsetv2 + uvec2(0, -1));
		B = gaussianSample(offsetv2 + uvec2(0, 1));
		L = gaussianSample(offsetv2 + uvec2(-1, 0));
		R = gaussianSample(offsetv2 + uvec2(1, 0));
	}
	else
	{
		T = dataSource[PixelCoordToDataOffset(offsetv2.x, offsetv2.y - 1)];
		B = dataSource[PixelCoordToDataOffset(offsetv2.x, offsetv2.y + 1)];
		L = dataSource[PixelCoordToDataOffset(offsetv2.x - 1, offsetv2.y)];
		R = dataSource[PixelCoordToDataOffset(offsetv2.x + 1, offsetv2.y)];
	}

	float slopeFactor = 0.0f;

	// calculate the slope factor
	
	float dX = (R - L);
	float dY = (B - T);
	slopeFactor = sqrt(dX * dX + dY * dY);

	return slopeFactor * u_Resolution / radius;
}

float calculateSlopeFactor()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;

	float factor = 0.0f;

	for (int i = -u_SlopeSmoothingRadius; i <= u_SlopeSmoothingRadius; i++)
	{
		for (int j = -u_SlopeSmoothingRadius; j <= u_SlopeSmoothingRadius; j++)
		{
			factor += calculateSlopeFactorAtCoord(offsetv2, uvec2(vec2(i, j) * u_SlopeSamplingRadius), u_SlopeSamplingRadius);
		}
	}

	factor /= (u_SlopeSmoothingRadius * 2 + 1) * (u_SlopeSmoothingRadius * 2 + 1);

	factor = smoothstep(u_TransformRange.x, u_TransformRange.y, factor);

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
	seed = seed * u_Frequency + u_Offset + vec3(u_Seed % 100);

	float n = 0.0f;
	float frequency = 1.0f;
	float amplitude = 1.0f;
	for (int i = 0 ; i < u_NoiseOctaveStrengthsCount ; i++)
	{
		n += tf3d_cnoise(seed * frequency) * amplitude * u_NoiseOctaveStrengths[i];
		frequency *= u_Lacunarity;
		amplitude *= u_Persistence;
	}


	if ( u_TransformFactor == 1) n = n * calculateSlopeFactor();
	else if ( u_TransformFactor == 2) n = n * smoothstep(u_TransformRange.x, u_TransformRange.y, dataSource[offset]);

	n = n * u_Strength * u_Influence;

	if ( u_MixMethod == 0 ) dataTarget[offset] = dataSource[offset] + n;
	else if ( u_MixMethod == 1 ) dataTarget[offset] = dataSource[offset] * n;
	else if ( u_MixMethod == 2 ) dataTarget[offset] = dataSource[offset] * n + dataSource[offset];
	else if ( u_MixMethod == 3 ) dataTarget[offset] = n;
	else dataTarget[offset] = dataSource[offset];

}

