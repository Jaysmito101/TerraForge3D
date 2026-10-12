#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/directional_guide.glsl"

uniform float u_DirectionAngle;
uniform float u_Radius;
uniform float u_Strength;

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
	float radius = max(clamp(u_Radius, 1.0f, 32.0f) * float(u_Resolution) / 1024.0f, 0.5f);
	float sigma = max(radius * 0.5f, 0.5f);
	float center = sampleInput(coordinate);
	float filtered = 0.0f;
	float weightSum = 0.0f;

	for (int tap = -8; tap <= 8; ++tap)
	{
		float offset = radius * float(tap) / 8.0f;
		float normalizedOffset = offset / sigma;
		vec2 sampleCoordinate = vec2(coordinate) + angleDirection * offset;
		float sampleValue = tf3dSampleFieldBilinear(sampleCoordinate);
		float spatialWeight = exp(-0.5f * normalizedOffset * normalizedOffset);
		float weight = spatialWeight;
		filtered += sampleValue * weight;
		weightSum += weight;
	}

	float strength = clamp(u_Strength, 0.0f, 1.0f) * slopeAlignment;
	writeOutput(coordinate, mix(center, filtered / max(weightSum, 0.000001f), strength));
}
