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

//
// GLSL textureless classic 3D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-10-11
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x)
{
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
	return mod289(((x * 34.0) + 10.0) * x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Classic Perlin noise
float cnoise(vec3 P)
{
	vec3 Pi0 = floor(P); // Integer part for indexing
	vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
	Pi0 = mod289(Pi0);
	Pi1 = mod289(Pi1);
	vec3 Pf0 = fract(P); // Fractional part for interpolation
	vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
	vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
	vec4 iy = vec4(Pi0.yy, Pi1.yy);
	vec4 iz0 = Pi0.zzzz;
	vec4 iz1 = Pi1.zzzz;

	vec4 ixy = permute(permute(ix) + iy);
	vec4 ixy0 = permute(ixy + iz0);
	vec4 ixy1 = permute(ixy + iz1);

	vec4 gx0 = ixy0 * (1.0 / 7.0);
	vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
	gx0 = fract(gx0);
	vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
	vec4 sz0 = step(gz0, vec4(0.0));
	gx0 -= sz0 * (step(0.0, gx0) - 0.5);
	gy0 -= sz0 * (step(0.0, gy0) - 0.5);

	vec4 gx1 = ixy1 * (1.0 / 7.0);
	vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
	gx1 = fract(gx1);
	vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
	vec4 sz1 = step(gz1, vec4(0.0));
	gx1 -= sz1 * (step(0.0, gx1) - 0.5);
	gy1 -= sz1 * (step(0.0, gy1) - 0.5);

	vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
	vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
	vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
	vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
	vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
	vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
	vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
	vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

	vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
	g000 *= norm0.x;
	g010 *= norm0.y;
	g100 *= norm0.z;
	g110 *= norm0.w;
	vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
	g001 *= norm1.x;
	g011 *= norm1.y;
	g101 *= norm1.z;
	g111 *= norm1.w;

	float n000 = dot(g000, Pf0);
	float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
	float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
	float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
	float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
	float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
	float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
	float n111 = dot(g111, Pf1);

	vec3 fade_xyz = fade(Pf0);
	vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
	vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
	float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
	return 2.2 * n_xyz;
}


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
		n += cnoise(seed * frequency) * amplitude * u_NoiseOctaveStrengths[i];
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

