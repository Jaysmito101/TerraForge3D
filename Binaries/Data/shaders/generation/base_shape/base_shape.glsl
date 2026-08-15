#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(TF3D_FIELD_FORMAT, binding = 0) writeonly uniform image2D DataTexture;

uniform int u_Resolution;
uniform bool u_UseSeedTexture;
uniform sampler2D u_SeedTexture;

/* TF3D_BASE_SHAPE_UNIFORMS */

/* TF3D_BASE_SHAPE_SOURCE */

void main()
{
	uvec2 invocation = gl_GlobalInvocationID.xy;
	if (invocation.x >= uint(u_Resolution) || invocation.y >= uint(u_Resolution))
		return;

	ivec2 pixelCoord = ivec2(invocation);
	vec2 uv = (vec2(invocation) + vec2(0.5f)) / float(u_Resolution);
	vec3 seed = vec3(uv * 2.0 - vec2(1.0), 0.0);
	if (u_UseSeedTexture) {
		seed = texture(u_SeedTexture, uv).rgb;
	}

	imageStore(DataTexture, pixelCoord, vec4(evaluateBaseShape(uv, seed), 0.0, 0.0, 0.0));
}
