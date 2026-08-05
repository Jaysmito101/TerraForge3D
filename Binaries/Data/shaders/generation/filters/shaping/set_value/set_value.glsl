#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_Value;
uniform int u_ValueMode;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float value = u_Value;
	if (u_ValueMode == 1) value = tf3dFieldDenormalize(u_Value);
	else if (u_ValueMode == 2) value = tf3dFieldRequestedPercentile();
	writeOutput(coordinate, value);
}
