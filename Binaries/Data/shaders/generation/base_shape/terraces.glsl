{
	"Name": "Terraces",
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
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.001, 16.0, 0.0, 0.0]
		},
		{
			"Name": "Levels",
			"Type": "Int",
			"Default": 8,
			"Widget": "Slider",
			"Constraints": [2.0, 32.0, 0.0, 0.0]
		},
		{
			"Name": "Smoothness",
			"Type": "Float",
			"Default": 0.35,
			"Widget": "Slider",
			"Constraints": [0.001, 1.0, 0.0, 0.0],
			"Tooltip": "Softens terrace transitions. A value of 1 approaches a continuous slope."
		},
		{
			"Name": "NoiseStrength",
			"Label": "Noise Strength",
			"Type": "Float",
			"Default": 0.15,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
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

vec2 tf3d_terraces_rotate(vec2 value, float angle)
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
	vec2 p = tf3d_terraces_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);

	int levels = clamp(u_Levels, 2, 32);
	float persistence = 0.5f;
	float source = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.8f, 6, 2.0f, persistence);
	float levelCoordinate = clamp(source, 0.0f, 1.0f) * float(levels);
	float levelIndex = floor(levelCoordinate);
	float levelPhase = fract(levelCoordinate);
	float smoothness = clamp(u_Smoothness, 0.001f, 1.0f);
	float transitionWidth = 0.5f * smoothness;
	float transition = tf3d_shape_smoothstep(
		0.5f - transitionWidth,
		0.5f + transitionWidth,
		levelPhase);
	float terrace = (levelIndex + transition) / float(levels);

	float detail = tf3d_terrain_fbm2(p + vec2(23.0f, 11.0f), 2.2f, 4, 2.0f, 0.5f);
	float noiseStrength = clamp(u_NoiseStrength, 0.0f, 1.0f);
	terrace += detail * noiseStrength * 0.08f * (0.35f + 0.65f * smoothness);
	return clamp(terrace, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}
