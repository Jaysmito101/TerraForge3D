#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Steps;
uniform float u_Offset;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float levels = float(max(u_Steps - 1, 1));
	float normalized = tf3dFieldNormalize(sampleInput(coordinate));
	float shifted = clamp(normalized + u_Offset, 0.0f, 1.0f);
	float stepped = floor(shifted * levels + 0.5f) / levels - u_Offset;
	writeOutput(coordinate, tf3dFieldDenormalize(stepped));
}
