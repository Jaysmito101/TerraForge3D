#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Radius;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	const int maximumRadius = 16;
	int radius = clamp(u_Radius, 1, maximumRadius);
	float surroundingHeight =
		sampleInput(coordinate + ivec2(-radius, 0))
		+ sampleInput(coordinate + ivec2(radius, 0))
		+ sampleInput(coordinate + ivec2(0, -radius))
		+ sampleInput(coordinate + ivec2(0, radius));

	outputData[indexOf(coordinate)] = surroundingHeight * 0.25f;
}
