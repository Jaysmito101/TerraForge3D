#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"
#include "common/base_shape_terrain_helpers.glsl"

vec2 tf3d_rolling_hills_rotate(vec2 value, float angle)
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
	vec2 p = tf3d_rolling_hills_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);

	int octaves = clamp(u_Octaves, 1, 16);
	float persistence = clamp(u_NoisePersistence, 0.0f, 0.95f);
	float lacunarity = clamp(u_NoiseLacunarity, 1.0f, 4.0f);
	float hills = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.72f, octaves, lacunarity, persistence);
	float ridges = tf3d_terrain_ridge2(p + vec2(19.0f, -7.0f), 0.52f, octaves, lacunarity, persistence);
	float ridgeAmount = clamp(u_RidgeAmount, 0.0f, 1.0f);
	float height = mix(hills, ridges, ridgeAmount);
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}
