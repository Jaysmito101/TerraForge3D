#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Axis;
uniform int u_Radius;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	int radius = clamp(u_Radius, 0, 64);
	float value = 0.0f;
	int sampleCount = 0;
	for (int offset = -64; offset <= 64; offset++)
	{
		if (abs(offset) > radius) continue;
		ivec2 sampleCoordinate = coordinate + (u_Axis == 0 ? ivec2(offset, 0) : ivec2(0, offset));
		value += sampleInput(sampleCoordinate);
		sampleCount++;
	}

	writeOutput(coordinate, value / max(float(sampleCount), 1.0f));
}
