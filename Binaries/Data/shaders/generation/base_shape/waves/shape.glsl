#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"
#include "common/base_shape_terrain_helpers.glsl"

vec2 tf3d_waves_rotate(vec2 value, float angle)
{
	float sine = sin(angle);
	float cosine = cos(angle);
	return mat2(cosine, -sine, sine, cosine) * value;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.001f);
	float rotation = 3.14159265f * clamp(u_Rotation, -360.0f, 360.0f) / 180.0f;
	vec2 offset = clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);
	vec2 p = tf3d_waves_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);

	float distortion = clamp(u_Distortion, 0.0f, 1.0f);
	vec2 warpDomain = p * 0.38f + seedOffset + vec2(7.0f, 19.0f);
	vec2 warp = vec2(
		tf3d_snoise2(warpDomain),
		tf3d_snoise2(warpDomain + vec2(23.0f, -13.0f))) * distortion * 0.45f;
	p += warp;

	float radialDistance = length(p);
	float phaseNoise = tf3d_terrain_fbm2(p + vec2(-11.0f, 31.0f), 0.34f, 4, 2.0f, 0.5f);
	float wavelength = clamp(u_Wavelength, 0.2f, 16.0f);
	float wave = 0.5f + 0.5f * sin(radialDistance * wavelength + phaseNoise * distortion * 1.4f);
	wave = tf3d_shape_smoothstep(0.08f, 0.92f, wave);
	float base = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.75f, clamp(u_Octaves, 1, 12), 2.0f, 0.5f);
	float amount = clamp(u_WaveAmount, 0.0f, 1.0f);
	float height = mix(base, wave, amount);
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}

