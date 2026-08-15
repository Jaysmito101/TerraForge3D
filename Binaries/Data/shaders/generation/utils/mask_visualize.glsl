#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D u_SourceMask;
layout(binding = 1, rgba32f) writeonly uniform image2D u_Destination;

uniform int u_Resolution;
uniform bool u_Invert;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) {
		return;
	}

	vec2 uv = (vec2(coordinate) + vec2(0.5)) / float(u_Resolution);
	float value = texture(u_SourceMask, uv).r;
	if (u_Invert) {
		value = -value;
	}

	float intensity = clamp(abs(value), 0.0f, 1.0f);
	vec3 neutral = vec3(0.5f);
	vec3 positive = vec3(1.0f, 0.24f, 0.08f);
	vec3 negative = vec3(0.08f, 0.35f, 1.0f);
	vec3 color = mix(neutral, value < 0.0f ? negative : positive, intensity);
	imageStore(u_Destination, coordinate, vec4(color, 1.0f));
}
