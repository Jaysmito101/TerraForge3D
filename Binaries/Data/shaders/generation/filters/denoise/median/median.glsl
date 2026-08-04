#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Radius;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	const int maximumRadius = 4;
	int radius = clamp(u_Radius, 0, maximumRadius);
	float samples[(maximumRadius * 2 + 1) * (maximumRadius * 2 + 1)];
	int count = 0;
	for (int y = -maximumRadius; y <= maximumRadius; y++)
	{
		for (int x = -maximumRadius; x <= maximumRadius; x++)
		{
			if (abs(x) > radius || abs(y) > radius) continue;
			samples[count++] = sampleInput(coordinate + ivec2(x, y));
		}
	}

	for (int i = 1; i < count; i++)
	{
		float value = samples[i];
		int j = i - 1;
		while (j >= 0 && samples[j] > value)
		{
			samples[j + 1] = samples[j];
			j--;
		}
		samples[j + 1] = value;
	}

	outputData[indexOf(coordinate)] = samples[count / 2];
}
