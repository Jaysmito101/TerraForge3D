#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"

vec2 rotate(vec2 value, float angle)
{
	float sine = sin(angle);
	float cosine = cos(angle);
	return mat2(cosine, -sine, sine, cosine) * value;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.001f);
	vec2 offset = clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);
	float rotation = 3.14159265f * clamp(u_Rotation, -36000.0f, 36000.0f) / 180.0f;
	vec2 p = rotate((uv * 2.0f - vec2(1.0f)) * scale * 2.5f + offset + seedOffset, rotation);
	float distortionScale = tf3d_shape_positive(u_DistortionScale, 0.001f);
	vec2 warpDomain = p * distortionScale * 0.35f + seedOffset;
	vec2 warp = vec2(
		tf3d_snoise2(warpDomain),
		tf3d_snoise2(warpDomain + vec2(17.0f, 5.0f)))
		* clamp(u_Distortion, 0.0f, 1.0f) * 0.75f;
	p += warp;

	vec2 duneDomain = vec2(p.x * 0.85f, p.y * 0.30f);
	float primary = tf3d_snoise2(duneDomain + seedOffset);
	float detail = tf3d_snoise2(duneDomain * 2.03f + seedOffset + vec2(13.7f, -4.2f));
	float ridgeSource = clamp(primary * 0.78f + detail * 0.22f, -1.0f, 1.0f);
	float ridge = 1.0f - abs(ridgeSource);
	float smoothness = clamp(u_Smoothness, 0.001f, 1.0f);
	float ridgeFloor = mix(0.16f, 0.02f, smoothness);
	float profile = tf3d_shape_smoothstep(ridgeFloor, 1.0f, ridge);
	profile = pow(profile, mix(1.35f, 0.85f, smoothness));
	float crossVariation = 0.5f + 0.5f * tf3d_snoise2(
		vec2(p.x * 0.22f, p.y * 0.65f) + seedOffset + vec2(31.0f, 7.0f));
	profile *= mix(0.82f, 1.0f, crossVariation);

	return clamp(profile, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}

