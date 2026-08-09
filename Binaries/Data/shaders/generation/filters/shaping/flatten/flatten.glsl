#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/terrain_filter_common.glsl"

uniform float u_Target;
uniform int u_TargetMode;
uniform int u_Radius;
uniform float u_LocalBlend;

float localAverage(ivec2 coordinate, int radius)
{
	float value = 0.0f;
	float weightSum = 0.0f;
	for (int y = -16; y <= 16; ++y)
	{
		for (int x = -16; x <= 16; ++x)
		{
			if (x * x + y * y > radius * radius) continue;
			float distance = length(vec2(x, y));
			float weight = 1.0f / (1.0f + distance);
			value += sampleInput(coordinate + ivec2(x, y)) * weight;
			weightSum += weight;
		}
	}
	return value / max(weightSum, 0.000001f);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	int radius = clamp(u_Radius, 1, 16);
	float target = u_Target;
	if (u_TargetMode == 0)
		target = tf3dFieldRequestedPercentile();
	else if (u_TargetMode == 1)
		target = tf3dFieldDenormalize(u_Target);
	else if (u_TargetMode == 3)
		target = localAverage(coordinate, radius);

	if (u_TargetMode != 3 && u_LocalBlend > 0.0f)
		target = mix(target, localAverage(coordinate, radius), clamp(u_LocalBlend, 0.0f, 1.0f));

	writeOutput(coordinate, target);
}
