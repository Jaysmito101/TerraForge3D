#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"
#include "../../../../common/base_shape_helpers.glsl"

uniform float u_Threshold;
uniform int u_Side;
uniform float u_Softness;

float clipValue(float value, float threshold, int side, float softness)
{
	if (softness <= 0.000001f) {
		return side == 0 ? max(value, threshold) : min(value, threshold);
	}

	return side == 0
		? tf3d_shape_smax(value, threshold, softness)
		: tf3d_shape_smin(value, threshold, softness);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float value = sampleInput(coordinate);
	outputData[indexOf(coordinate)] = clipValue(value, u_Threshold, u_Side, max(u_Softness, 0.0f));
}
