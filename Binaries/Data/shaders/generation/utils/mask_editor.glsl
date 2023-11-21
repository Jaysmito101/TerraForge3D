#version 430 core

// work group size
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// texture for mask
layout(binding = 0, rgba32f) uniform image2D u_MaskTexture;


uniform int u_Resolution;
uniform vec2 u_BrushPosition;
uniform vec4 u_BrushSettings0;
uniform int u_Mode;


float calculateFallOff(in vec2 uv)
{
	float distanceVal = length(uv - u_BrushPosition);
	return smoothstep(u_BrushSettings0.y * (1.0f - u_BrushSettings0.z), u_BrushSettings0.y, distanceVal);
}

void main()
{
	ivec2 offset = ivec2(gl_GlobalInvocationID.xy);
	vec4 oriVal = imageLoad(u_MaskTexture, offset);

	vec2 uv = offset / float(u_Resolution);

	// u_BrushSettings0 = strength, size, falloff, reserved
	float val = mix(u_BrushSettings0.x, 0.0f, calculateFallOff(uv));

	if (u_Mode == 1)
		val = clamp(oriVal.x - val, 0.0f, 1.0f);
	else
		val = clamp(oriVal.x + val, 0.0f, 1.0f);

	imageStore(u_MaskTexture, offset, vec4(val, val, val, 1.0f));
}
