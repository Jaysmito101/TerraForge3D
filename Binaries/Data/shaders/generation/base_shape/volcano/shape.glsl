#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"


float noise(vec2 uv)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float fac = 1.0f;
	float f  = fac * 0.5000f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.2500f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.1250f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.0625f * tf3d_snoise2( uv ); uv = m*uv;
	return f;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float radius = tf3d_shape_positive(u_Radius, 0.05f);
	float mountainFalloff = tf3d_shape_positive(u_MountainFalloff, 0.01f);
	float distortionScale = tf3d_shape_positive(u_DistortionScale, 0.001f);
	float outsideNoiseScale = tf3d_shape_positive(u_OutsideNoiseScale, 0.001f);
	vec2 offset = clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);
	uv = uv * 2.0f - vec2(1.0f) + offset;

	float distortion = noise(uv * distortionScale + seedOffset)
		* clamp(u_Distortion, 0.0f, 0.5f) * radius * 0.35f;
	float r = length(uv) + distortion;
	float craterSize = clamp(abs(u_CraterRadius) * 0.3f, 0.0f, 0.95f);
	float craterRadius = max(radius * craterSize, 0.001f);
	float craterEnabled = step(0.001f, craterSize);
	float mountainOuter = radius + mountainFalloff;

	float mountainMask = 1.0f - tf3d_shape_smoothstep(craterRadius * 0.5f, mountainOuter, r);
	float craterMask = craterEnabled * (1.0f - tf3d_shape_smoothstep(0.0f, craterRadius, r));
	float ns = clamp(u_Height, 0.0f, 4.0f) * mountainMask
		- clamp(u_Height, 0.0f, 4.0f) * clamp(u_CraterDepth, 0.0f, 4.0f) * 0.3f * craterMask;

	float outsideMask = tf3d_shape_smoothstep(craterRadius, mountainOuter, r);
	ns += tf3d_snoise2(uv * outsideNoiseScale + seedOffset) * 0.2f
		* clamp(u_OutsideNoise, 0.0f, 1.0f) * outsideMask;
	return ns * clamp(u_Strength, 0.0f, 4.0f);
}

