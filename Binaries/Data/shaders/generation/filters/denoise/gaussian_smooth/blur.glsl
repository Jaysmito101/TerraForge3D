#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "common.glsl"

uniform int u_Axis;
uniform int u_Radius;

float gaussianWeight(int offset)
{
	float sigma = max(float(u_Radius) * 0.5f, 0.5f);
	return exp(-0.5f * float(offset * offset) / (sigma * sigma));
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;
	int radius = clamp(u_Radius, 0, 32);

	float value = 0.0f;
	float weightSum = 0.0f;
	for (int offset = -radius; offset <= radius; ++offset)
	{
		float weight = gaussianWeight(offset);
		ivec2 sampleCoordinate = coordinate + (u_Axis == 0 ? ivec2(offset, 0) : ivec2(0, offset));
		value += sampleInput(sampleCoordinate) * weight;
		weightSum += weight;
	}
	outputData[indexOf(coordinate)] = value / max(weightSum, 0.000001f);
}
