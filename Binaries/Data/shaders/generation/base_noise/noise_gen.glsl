#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D DataSourceTexture;
layout(TF3D_FIELD_FORMAT, binding = 1) writeonly uniform image2D DataTargetTexture;

// uniforms
uniform int u_Resolution;
uniform bool u_UseSeedTexture;
uniform sampler2D u_SeedTexture;
uniform int u_NoiseAlgorithm;
uniform int u_NoiseSeed;
uniform float u_NoiseFrequency;
uniform int u_NoiseOctaves;
uniform float u_NoiseWarp;
uniform float u_NoiseJitter;
uniform float u_Strength;
uniform float u_Influence;
uniform float u_NoiseLacunarity;
uniform float u_NoisePersistence;
uniform int u_MixMethod;
uniform vec3 u_NoiseOffset;
uniform float u_NoiseOctaveStrengths[16];
uniform int u_NoiseOctaveStrengthsCount;
uniform bool u_UseMask;
uniform bool u_InvertMask;
layout(binding = 3) uniform sampler2D u_MaskTexture;

#include "common/noise_2d.glsl"

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;

	if (offsetv2.x >= uint(u_Resolution) || offsetv2.y >= uint(u_Resolution)) return;

	vec2 uv = (vec2(offsetv2) + vec2(0.5f)) / float(u_Resolution);
	vec3 seed = vec3(uv * 2.0f - vec2(1.0f), 0.0f);
	if (u_UseSeedTexture)
	{
		seed = texture(u_SeedTexture, uv).rgb; 
	}
	float frequencyInput = clamp(abs(u_NoiseFrequency), 0.001f, max(float(u_Resolution), 1.0f));
	float lacunarity = clamp(u_NoiseLacunarity, 1.0f, 4.0f);
	float persistence = clamp(u_NoisePersistence, 0.0f, 1.0f);
	vec3 offsetInput = clamp(u_NoiseOffset, vec3(-10000.0f), vec3(10000.0f));
	seed = seed * frequencyInput + offsetInput + vec3(u_NoiseSeed % 100);

	vec2 noiseDomain = seed.xy;
	float safeWarp = clamp(abs(u_NoiseWarp), 0.0f, 4.0f);
	if (safeWarp > 0.0001f)
	{
		noiseDomain += vec2(
			tf3d_noise2(noiseDomain * 0.5f + vec2(17.0f, 5.0f), u_NoiseAlgorithm, u_NoiseJitter, float(u_NoiseSeed) + 13.0f),
			tf3d_noise2(noiseDomain * 0.5f + vec2(-7.0f, 23.0f), u_NoiseAlgorithm, u_NoiseJitter, float(u_NoiseSeed) + 37.0f)) * safeWarp;
	}

	float n = 0.0f;
	float amplitude = 1.0f;
	float amplitudeSum = 0.0f;
	int octaveCount = clamp(min(u_NoiseOctaves, u_NoiseOctaveStrengthsCount), 0, 16);
	const mat2 octaveRotation = mat2(0.8f, -0.6f, 0.6f, 0.8f);
	float octaveFrequency = frequencyInput;
	for (int i = 0; i < octaveCount; ++i)
	{
		float octaveStrength = clamp(u_NoiseOctaveStrengths[i], 0.0f, 1.0f);
		float pixelsPerFeature = float(u_Resolution) / max(2.0f * octaveFrequency, 0.0001f);
		float antiAliasWeight = smoothstep(2.0f, 4.0f, pixelsPerFeature);
		float effectiveStrength = octaveStrength * antiAliasWeight;
		n += tf3d_noise2(noiseDomain, u_NoiseAlgorithm, u_NoiseJitter, float(u_NoiseSeed) + float(i) * 11.73f)
			* amplitude * effectiveStrength;
		amplitudeSum += amplitude * effectiveStrength;
		noiseDomain = octaveRotation * noiseDomain * lacunarity + vec2(17.13f, 9.71f);
		amplitude *= persistence;
		octaveFrequency *= lacunarity;
	}
	n /= max(amplitudeSum, 0.0001f);

	float sourceValue = imageLoad(DataSourceTexture, ivec2(offsetv2)).r;
	float noiseValue = n * clamp(u_Strength, -4.0f, 4.0f);
	float targetValue = sourceValue;
	if (u_MixMethod == 0) targetValue = sourceValue + noiseValue;
	else if (u_MixMethod == 1) targetValue = sourceValue * noiseValue;
	else if (u_MixMethod == 2) targetValue = sourceValue * noiseValue + sourceValue;
	else if (u_MixMethod == 3) targetValue = noiseValue;

	float maskValue = 1.0f;
	if (u_UseMask)
	{
		maskValue = texelFetch(u_MaskTexture, ivec2(offsetv2), 0).r;
		if (u_InvertMask) maskValue = 1.0f - maskValue;
	}
	float influence = clamp(u_Influence * maskValue, 0.0f, 1.0f);
	float result = mix(sourceValue, targetValue, influence);
	imageStore(DataTargetTexture, ivec2(offsetv2), vec4(result, 0.0, 0.0, 0.0));

}
