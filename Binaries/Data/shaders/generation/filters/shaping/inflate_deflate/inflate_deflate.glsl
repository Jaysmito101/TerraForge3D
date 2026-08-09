#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/terrain_filter_common.glsl"

uniform int u_Mode;
uniform int u_Radius;
uniform float u_SlopeLimit;
uniform float u_SlopeSoftness;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float center = sampleInput(coordinate);
	vec2 uphill = tf3dFieldDirection(tf3dFieldGradient(coordinate));
	vec2 direction = u_Mode == 0 ? uphill : -uphill;
	int radius = clamp(u_Radius, 1, 32);
	float extreme = center;

	for (int offset = 1; offset <= 32; ++offset)
	{
		if (offset > radius) break;
		float sampleValue = tf3dSampleFieldBilinear(vec2(coordinate) + direction * float(offset));
		extreme = u_Mode == 0 ? max(extreme, sampleValue) : min(extreme, sampleValue);
	}

	float slope = tf3dFieldSlope(coordinate);
	float softness = max(u_SlopeSoftness, 0.000001f);
	float slopeInfluence = smoothstep(max(u_SlopeLimit, 0.0f), max(u_SlopeLimit, 0.0f) + softness, slope);
	writeOutput(coordinate, mix(center, extreme, slopeInfluence));
}
