#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_PeakThreshold;
uniform float u_Sharpness;
uniform float u_Feather;
uniform float u_NoiseFrequency;
uniform int u_NoiseOctaves;
uniform float u_DetailStrength;

#define u_NoiseSeed 152
#define u_NoiseAlgorithm TF3D_NOISE_SIMPLEX
#define u_NoiseWarp 0.0f
#define u_NoiseJitter 0.0f
#include "../../../../common/noise_2d.glsl"

float evaluateRidgedDetail(vec2 uv)
{
	int octaveCount = clamp(u_NoiseOctaves, 1, 8);
	float frequency = clamp(abs(u_NoiseFrequency), 0.001f, max(float(u_Resolution) / 8.0f, 0.001f));
	vec2 domain = (uv * 2.0f - 1.0f) * frequency;
	const mat2 octaveRotation = mat2(0.8f, -0.6f, 0.6f, 0.8f);
	float detail = 0.0f;
	float amplitude = 1.0f;
	float amplitudeSum = 0.0f;

	for (int octave = 0; octave < 8; ++octave)
	{
		if (octave >= octaveCount)
		{
			break;
		}

		float pixelsPerFeature = float(u_Resolution) / max(2.0f * frequency, 0.0001f);
		float antiAliasWeight = smoothstep(2.0f, 4.0f, pixelsPerFeature);
		if (octave > 0 && pixelsPerFeature <= 2.0f)
		{
			break;
		}

		float noise = tf3d_noise2(domain, TF3D_NOISE_SIMPLEX, 0.0f,
			float(u_NoiseSeed) + float(octave) * 11.73f);
		float ridge = 1.0f - abs(noise);
		float ridgeSignal = 2.0f * smoothstep(0.0f, 1.0f, ridge) - 1.0f;
		detail += ridgeSignal * amplitude * antiAliasWeight;
		amplitudeSum += amplitude * antiAliasWeight;

		domain = octaveRotation * domain * 2.0f + vec2(17.13f, 9.71f);
		frequency *= 2.0f;
		amplitude *= 0.5f;
	}

	return detail / max(amplitudeSum, 0.0001f);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution)
	{
		return;
	}

	float normalizedHeight = tf3dFieldNormalize(sampleInput(coordinate));
	float threshold = clamp(u_PeakThreshold, 0.0f, 0.9999f);
	float peakProgress = clamp((normalizedHeight - threshold) / (1.0f - threshold), 0.0f, 1.0f);
	float feather = clamp(u_Feather, 0.0f, 1.0f - threshold);
	float peakInfluence = feather > 0.0001f
		? smoothstep(threshold, threshold + feather, normalizedHeight)
		: step(threshold, normalizedHeight);
	float sharpenedPeak = threshold + pow(peakProgress, max(u_Sharpness, 1.0f)) * (1.0f - threshold);
	float normalizedResult = mix(normalizedHeight, sharpenedPeak, peakInfluence);
	float resultProgress = clamp((normalizedResult - threshold) / (1.0f - threshold), 0.0f, 1.0f);
	float detailEnvelope = (1.0f - threshold) * resultProgress * (1.0f - resultProgress) * peakInfluence;
	normalizedResult += evaluateRidgedDetail((vec2(coordinate) + 0.5f) / float(u_Resolution))
		* clamp(u_DetailStrength, 0.0f, 1.0f) * detailEnvelope;

	writeOutput(coordinate, tf3dFieldDenormalize(normalizedResult));
}
