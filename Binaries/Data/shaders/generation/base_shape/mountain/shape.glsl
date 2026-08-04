#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"


float ridgeNoise(vec2 uv)
{
	float ridge = 1.0f - abs(clamp(tf3d_snoise2(uv), -1.0f, 1.0f));
	return smoothstep(0.0f, 1.0f, clamp(ridge, 0.0f, 1.0f));
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float scale = tf3d_shape_positive(u_Scale, 0.001f);
	vec2 offset = clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);
	vec2 p = uv * scale + offset + seedOffset;
	float ns = 0.0f;
	float amplitude = 1.0f;
	float amplitudeSum = 0.0f;
	float damping = clamp(u_DampingFactor, 0.0f, 0.99f);
	int levels = clamp(u_Levels, 1, 24);
	for(int i = 0 ; i < levels ; i++)
	{
		float octaveProgress = float(i) / float(max(levels - 1, 1));
		float octaveWeight = mix(1.0f, 0.65f, octaveProgress);
		ns += ridgeNoise(p) * amplitude * octaveWeight;
		amplitudeSum += amplitude;
		p = m * p + vec2(17.13f, -9.71f) * float(i + 1);
		amplitude *= damping;
	}
	return clamp(ns / max(amplitudeSum, TF3D_SHAPE_EPSILON), 0.0f, 1.0f)
		* clamp(u_Strength, 0.0f, 4.0f);
}

