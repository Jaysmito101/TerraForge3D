#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/terrain_filter_common.glsl"

uniform vec2 u_Center;
uniform float u_Turns;
uniform float u_Radius;
uniform float u_Falloff;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	vec2 uv = (vec2(coordinate) + vec2(0.5f)) / float(u_Resolution);
	vec2 centered = uv - u_Center;
	float radius = max(u_Radius, 0.000001f);
	float distanceToCenter = length(centered);
	float radialInfluence = 1.0f - smoothstep(0.0f, radius, distanceToCenter);
	radialInfluence = pow(clamp(radialInfluence, 0.0f, 1.0f), max(u_Falloff, 0.0001f));

	float angle = u_Turns * 6.28318530718f * radialInfluence;
	float sine = sin(-angle);
	float cosine = cos(-angle);
	vec2 rotated = vec2(
		centered.x * cosine - centered.y * sine,
		centered.x * sine + centered.y * cosine);
	vec2 sourceUv = u_Center + rotated;
	vec2 sourceCoordinate = sourceUv * float(u_Resolution) - vec2(0.5f);
	writeOutput(coordinate, tf3dSampleFieldBilinear(sourceCoordinate));
}
