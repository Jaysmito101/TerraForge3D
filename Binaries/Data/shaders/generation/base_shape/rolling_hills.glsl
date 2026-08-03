{
	"Name": "Rolling Hills",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.8,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.2,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.001, 16.0, 0.0, 0.0]
		},
		{
			"Name": "Octaves",
			"Type": "Int",
			"Default": 5,
			"Widget": "Slider",
			"Constraints": [1.0, 16.0, 0.0, 0.0]
		},
		{
			"Name": "Persistence",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Slider",
			"Constraints": [0.0, 0.95, 0.0, 0.0]
		},
		{
			"Name": "Lacunarity",
			"Type": "Float",
			"Default": 2.0,
			"Widget": "Slider",
			"Constraints": [1.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "RidgeAmount",
			"Label": "Ridge Amount",
			"Type": "Float",
			"Default": 0.15,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0],
			"Tooltip": "Blends broad hills toward softer ridges without introducing hard seams."
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 42,
			"Widget": "Seed"
		},
		{
			"Name": "Offset",
			"Type": "Vector2",
			"Default": [0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [-8.0, 8.0, 0.0, 0.0]
		},
		{
			"Name": "Rotation",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Constraints": [-180.0, 180.0, 0.0, 0.0]
		}
	]
}
// CODE

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
	float persistence = clamp(u_Persistence, 0.0f, 0.95f);
	float lacunarity = clamp(u_Lacunarity, 1.0f, 4.0f);
	float hills = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.72f, octaves, lacunarity, persistence);
	float ridges = tf3d_terrain_ridge2(p + vec2(19.0f, -7.0f), 0.52f, octaves, lacunarity, persistence);
	float ridgeAmount = clamp(u_RidgeAmount, 0.0f, 1.0f);
	float height = mix(hills, ridges, ridgeAmount);
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}
