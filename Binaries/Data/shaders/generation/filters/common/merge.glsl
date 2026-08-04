#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "field_common.glsl"

layout(std430, binding = 1) readonly buffer OperationData
{
	float operationData[];
};

layout(binding = 3) uniform sampler2D u_MaskTexture;

uniform float u_Strength;
uniform int u_MergeMode;
uniform bool u_UseMask;
uniform bool u_InvertMask;
// Optional sentinel used by iterative filters whose result may be incomplete
// when their iteration budget is smaller than the terrain dimensions.
uniform float u_InvalidOperationThreshold;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	uint index = indexOf(coordinate);
	float inputValue = inputData[index];
	float operationValue = operationData[index];
	if (u_InvalidOperationThreshold > 0.0 && abs(operationValue) >= u_InvalidOperationThreshold)
		operationValue = inputValue;
	float maskValue = u_UseMask ? texelFetch(u_MaskTexture, coordinate, 0).r : 1.0f;
	if (u_UseMask && u_InvertMask) maskValue = 1.0f - maskValue;
	float influence = clamp(u_Strength * maskValue, 0.0f, 1.0f);
	float target = operationValue;
	if (u_MergeMode == 1) target = inputValue + operationValue;
	else if (u_MergeMode == 2) target = inputValue - operationValue;
	else if (u_MergeMode == 3) target = inputValue * operationValue;
	outputData[index] = mix(inputValue, target, influence);
}
