#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"
#include "../../../../common/base_shape_helpers.glsl"

uniform float u_Threshold;
uniform int u_Side;
uniform float u_Softness;
uniform int u_ThresholdMode;

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
	float threshold = u_Threshold;
	float softness = max(u_Softness, 0.0f);
	if (u_ThresholdMode == 1)
	{
		threshold = mix(tf3dFieldMinimum(), tf3dFieldMaximum(), clamp(u_Threshold, 0.0f, 1.0f));
		softness *= tf3dFieldScaleRange();
	}
	else if (u_ThresholdMode == 2)
	{
		threshold = tf3dFieldRequestedPercentile();
		softness *= tf3dFieldScaleRange();
	}
	writeOutput(coordinate, clipValue(value, threshold, u_Side, softness));
}
