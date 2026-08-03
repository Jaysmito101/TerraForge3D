{
	"Name": "Karst",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 2.0,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.1, 16.0, 0.0, 0.0]
		},
		{
			"Name": "CavitySize",
			"Label": "Cavity Size",
			"Type": "Float",
			"Default": 0.4,
			"Widget": "Slider",
			"Constraints": [0.08, 0.85, 0.0, 0.0]
		},
		{
			"Name": "CavityDepth",
			"Label": "Cavity Depth",
			"Type": "Float",
			"Default": 0.65,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Softness",
			"Type": "Float",
			"Default": 0.4,
			"Widget": "Slider",
			"Constraints": [0.001, 1.0, 0.0, 0.0]
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

vec2 tf3d_karst_rotate(vec2 value, float angle)
{
	float sine = sin(angle);
	float cosine = cos(angle);
	return mat2(cosine, -sine, sine, cosine) * value;
}

float tf3d_karst_cavityField(vec2 p, float cavitySize, float softness)
{
	vec2 cell = floor(p);
	vec2 local = fract(p);
	float field = 0.0f;
	float radiusBase = clamp(cavitySize, 0.08f, 0.85f);
	float edge = mix(0.006f, 0.22f, clamp(softness, 0.001f, 1.0f));

	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			vec2 gridOffset = vec2(float(x), float(y));
			vec2 cellId = cell + gridOffset;
			vec2 feature = tf3d_terrain_hash22(cellId) - vec2(0.5f);
			vec2 delta = gridOffset + feature * 0.72f - local;
			float randomSize = tf3d_terrain_hash22(cellId + vec2(5.3f, 21.7f)).y;
			float radius = radiusBase * mix(0.72f, 1.18f, randomSize);
			float cavity = 1.0f - tf3d_shape_smoothstep(radius, radius + edge, length(delta));
			field = tf3d_shape_smax(field, cavity, 0.055f);
		}
	}

	return clamp(field, 0.0f, 1.0f);
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.1f);
	float rotation = 3.14159265f * clamp(u_Rotation, -360.0f, 360.0f) / 180.0f;
	vec2 offset = clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);
	vec2 p = tf3d_karst_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);
	float cavities = tf3d_karst_cavityField(p, u_CavitySize, u_Softness);
	float base = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.85f, 5, 2.0f, 0.5f);
	float floorNoise = 0.5f + 0.5f * tf3d_terrain_fbm2(p + vec2(17.0f, -29.0f), 1.7f, 3, 2.0f, 0.5f);
	float floorHeight = 0.08f + 0.20f * floorNoise;
	float depth = clamp(u_CavityDepth, 0.0f, 1.0f);
	float height = mix(base, floorHeight, cavities * depth);
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}
