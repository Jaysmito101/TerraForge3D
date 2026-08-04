#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D u_SourceMask;
layout(binding = 1, r16) uniform image2D u_DestinationMask;

uniform int u_Resolution;
uniform bool u_Invert;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	vec2 uv = (vec2(coordinate) + vec2(0.5)) / float(u_Resolution);
	float value = texture(u_SourceMask, uv).r;
	if (u_Invert) value = 1.0f - value;
	imageStore(u_DestinationMask, coordinate, vec4(value, 0.0, 0.0, 1.0));
}
