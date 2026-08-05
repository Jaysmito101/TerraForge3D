#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_Exponent;
uniform bool u_Abs;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float value = tf3dFieldNormalize(sampleInput(coordinate));
	if (u_Abs) value = abs(value);
	float exponent = max(u_Exponent, 0.0001f);
	writeOutput(coordinate, tf3dFieldDenormalize(pow(max(value, 0.0f), exponent)));
}
