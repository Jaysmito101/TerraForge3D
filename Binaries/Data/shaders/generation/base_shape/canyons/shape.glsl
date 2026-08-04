#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"
#include "common/base_shape_terrain_helpers.glsl"

vec2 tf3d_canyons_rotate(vec2 value, float angle)
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
	vec2 p = tf3d_canyons_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);

	float waviness = clamp(u_Waviness, 0.0f, 1.0f);
	vec2 warpDomain = p * 0.42f + seedOffset + vec2(17.0f, 3.0f);
	vec2 warp = vec2(
		tf3d_snoise2(warpDomain),
		tf3d_snoise2(warpDomain + vec2(29.0f, -11.0f))) * waviness * 0.55f;
	p += warp;

	int octaves = clamp(u_Octaves, 1, 16);
	float terrain = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.72f, octaves, 2.0f, 0.5f);
	float canyonSignal = tf3d_snoise2(vec2(p.x * 0.42f, p.y * 1.15f) + seedOffset);
	float width = clamp(u_Width, 0.01f, 0.9f);
	float canyonMask = 1.0f - tf3d_shape_smoothstep(0.0f, width, abs(canyonSignal));
	float floorNoise = 0.5f + 0.5f * tf3d_terrain_fbm2(p + vec2(-19.0f, 27.0f), 1.8f, 4, 2.0f, 0.5f);
	float canyonFloor = 0.08f + 0.22f * floorNoise;
	float depth = clamp(u_Depth, 0.0f, 1.0f);
	float height = mix(terrain, canyonFloor, canyonMask * depth);
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}

