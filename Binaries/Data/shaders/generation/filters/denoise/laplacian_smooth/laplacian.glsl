#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_Weight;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float center = sampleInput(coordinate);
	float neighbors = 0.25f * (
		sampleInput(coordinate + ivec2(-1, 0))
		+ sampleInput(coordinate + ivec2(1, 0))
		+ sampleInput(coordinate + ivec2(0, -1))
		+ sampleInput(coordinate + ivec2(0, 1)));
	float weight = clamp(u_Weight, 0.0f, 0.25f);
	outputData[indexOf(coordinate)] = center + (neighbors - center) * weight;
}
