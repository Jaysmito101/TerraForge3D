#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform float u_CoreStrength;
uniform float u_Curvature;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	vec2 uv = (vec2(coordinate) + vec2(0.5f)) / float(u_Resolution);
	vec2 centered = abs(uv * 2.0f - 1.0f);
	float curvature = max(u_Curvature, 0.0001f);

	float normalizedRadiusSquared = clamp(dot(centered, centered) * 0.5f, 0.0f, 1.0f);
	float edgeValue = exp(-curvature);
	float gaussian = exp(-curvature * normalizedRadiusSquared);
	float dome = (gaussian - edgeValue) / max(1.0f - edgeValue, 0.0001f);

	outputData[indexOf(coordinate)] = clamp(dome * u_CoreStrength, 0.0f, 1.0f);
}
