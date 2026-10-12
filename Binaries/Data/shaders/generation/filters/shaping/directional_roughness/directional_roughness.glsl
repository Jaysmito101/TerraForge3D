#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/directional_guide.glsl"

#define u_NoiseFrequency 1.0f
#define u_NoiseAlgorithm TF3D_NOISE_SIMPLEX
#define u_NoiseSeed 173
#define u_NoiseWarp 0.0f
#define u_NoiseJitter 0.0f
#include "../../../../common/noise_2d.glsl"

uniform float u_DirectionAngle;
uniform float u_Frequency;
uniform int u_Octaves;
uniform float u_Strength;

const float kReferenceResolution = 1024.0f;

float evaluateDirectionalRoughness(vec2 terrainPosition, vec2 direction)
{
	int octaveCount = clamp(u_Octaves, 1, 6);
	float frequency = clamp(u_Frequency, 32.0f, 512.0f) * float(u_Resolution) / kReferenceResolution;
	vec2 acrossDirection = vec2(-direction.y, direction.x);
	vec2 domain = vec2(
		dot(terrainPosition, direction) * 0.5f,
		dot(terrainPosition, acrossDirection)) * frequency;
	float detail = 0.0f;
	float amplitude = 1.0f;
	float amplitudeEnergy = 0.0f;

	for (int octave = 0; octave < 6; ++octave)
	{
		if (octave >= octaveCount)
		{
			break;
		}

		float pixelsPerFeature = float(u_Resolution) / max(frequency, 0.0001f);
		float antiAliasWeight = smoothstep(1.5f, 3.0f, pixelsPerFeature);
		if (antiAliasWeight <= 0.0001f)
		{
			break;
		}

		float noise = tf3d_noise2(domain, TF3D_NOISE_PERLIN, 0.0f, 173.0f + float(octave) * 19.19f);
		float weightedAmplitude = amplitude * antiAliasWeight;
		detail += noise * weightedAmplitude;
		amplitudeEnergy += weightedAmplitude * weightedAmplitude;

		domain = domain * 2.0f + vec2(17.13f, 9.71f);
		frequency *= 2.0f;
		amplitude *= 0.5f;
	}

	return clamp(detail / sqrt(max(amplitudeEnergy, 0.00000001f)), -1.0f, 1.0f);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution)
	{
		return;
	}

	vec2 angleDirection = tf3dAngleDirection(u_DirectionAngle);
	vec2 upwardSlope = tf3dUpwardSlopeAt(coordinate);
	float slopeAlignment = tf3dUpwardSlopeAlignment(upwardSlope, angleDirection);
	vec2 terrainPosition = (vec2(coordinate) + 0.5f) / float(u_Resolution) - 0.5f;
	float current = sampleInput(coordinate);
	float detail = evaluateDirectionalRoughness(terrainPosition, angleDirection);
	float normalizedSlope = length(upwardSlope) * float(u_Resolution) / tf3dFieldFullRange();
	float referenceFrequency = clamp(u_Frequency, 32.0f, 512.0f);
	float localFeatureRelief = normalizedSlope / max(referenceFrequency, 1.0f);
	float strength = clamp(u_Strength, 0.0f, 0.5f);
	float localReliefLimit = localFeatureRelief * 3.0f;
	float effectiveStrength = min(strength, localReliefLimit);
	writeOutput(coordinate, current + detail * effectiveStrength * tf3dFieldFullRange() * slopeAlignment);
}
