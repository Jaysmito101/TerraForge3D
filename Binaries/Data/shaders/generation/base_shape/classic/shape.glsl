#include "common/noise_3d.glsl"
#include "common/base_shape_helpers.glsl"

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.001);
	vec3 offset = clamp(u_Offset, vec3(-10000.0), vec3(10000.0));
	seed = (seed + offset) * scale + vec3(u_Seed);
	float n = 0.0f;
	int levels = clamp(u_Levels, 0, 6);
	for(int i = 0 ; i < levels ; i++)
	{
		seed = vec3(tf3d_cnoise(seed + vec3(0.0f, 0.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(1.0f, 2.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(3.0f, 4.0f, 0.0f))
		);
	}
	n = clamp(tf3d_cnoise(seed), -1.0f, 1.0f);
	if(u_AbsoluteValue) n = abs(n);
	if(u_SquareValue) n = n * n;
	return n * clamp(u_Strength, -4.0f, 4.0f);
}
