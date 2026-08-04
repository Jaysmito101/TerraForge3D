#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Radius;
uniform float u_Epsilon;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	int radius = clamp(u_Radius, 1, 8);
	float mean = 0.0f;
	float meanSquare = 0.0f;
	int sampleCount = 0;
	for (int y = -8; y <= 8; y++)
	{
		for (int x = -8; x <= 8; x++)
		{
			if (abs(x) > radius || abs(y) > radius) continue;
			float value = sampleInput(coordinate + ivec2(x, y));
			mean += value;
			meanSquare += value * value;
			sampleCount++;
		}
	}
	mean /= max(float(sampleCount), 1.0f);
	meanSquare /= max(float(sampleCount), 1.0f);

	float variance = max(meanSquare - mean * mean, 0.0f);
	float epsilon = max(u_Epsilon, 0.000001f);
	float coefficient = variance / (variance + epsilon);
	float offset = mean - coefficient * mean;
	float center = sampleInput(coordinate);
	writeOutput(coordinate, coefficient * center + offset);
}
