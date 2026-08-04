#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"


vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float noise(vec2 uv)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float fac = 0.16f;
	float f  = fac * 0.5000f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.2500f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.1250f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.0625f * tf3d_snoise2( uv ); uv = m*uv;
	return f;
}


float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.14159265f * clamp(u_Rotation, -36000.0f, 36000.0f) / 180.0f;
	uv = 2.0f * uv - vec2(1.0f);
	uv = rotate(uv, rotation);
	vec2 position = clamp(u_Position, vec2(-1.0f), vec2(1.0f));
	float rad = tf3d_shape_positive(u_Radius, 0.03f);
	float outerFalloff = clamp(abs(u_OuterFalloff), 0.01f, 1.0f);
	float innerFalloff = clamp(u_InnerFalloff, 0.0f, 0.999f);
	vec2 seedOffset = vec2(float(u_Seed) * 0.173f, float(u_Seed) * 0.317f);

	float largeNoise = tf3d_snoise2(uv * 1.5f + seedOffset);
	float smallNoise = noise(uv * 2.0f + seedOffset);
	float radiusNoise = largeNoise * clamp(u_LargeDistortion, 0.0f, 1.0f) * 0.2f
		+ smallNoise * clamp(u_SmallDistortion, 0.0f, 1.0f);
	float r = length(uv - position) - radiusNoise;

	float outerMask = 1.0f - tf3d_shape_smoothstep(rad, rad * (1.0f + outerFalloff), r);
	float basinMask = 1.0f - tf3d_shape_smoothstep(rad * innerFalloff, rad, r);
	float ns0 = clamp(u_Height, 0.0f, 2.0f) * outerMask
		- clamp(u_Depth, 0.0f, 2.0f) * basinMask;

	float insideMask = 1.0f - tf3d_shape_smoothstep(rad * innerFalloff, rad, r);
	float outsideMask = tf3d_shape_smoothstep(rad, rad * (1.0f + outerFalloff), r);
	float ns1 = largeNoise * clamp(u_LargeNoise, 0.0f, 1.0f) * 0.2f
		+ smallNoise * clamp(u_SmallNoise, 0.0f, 1.0f);
	float ns2 = ns1 * (clamp(u_InsideNoise, 0.0f, 1.0f) * insideMask
		+…5097 tokens truncated…"Drag",
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
			"Name": "PlateauWidth",
			"Label": "Plateau Width",
			"Type": "Float",
			"Default": 0.6,
			"Widget": "Slider",
			"Constraints": [0.05, 0.95, 0.0, 0.0]
		},
		{
			"Name": "EdgeSoftness",
			"Label": "Edge Softness",
			"Type": "Float",
			"Default": 0.35,
			"Widget": "Slider",
			"Constraints": [0.001, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Erosion",
			"Type": "Float",
			"Default": 0.3,
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

vec2 tf3d_mesas_rotate(vec2 value, float angle)
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
	vec2 p = tf3d_mesas_rotate((uv * 2.0f - vec2(1.0f)) * scale + offset + seedOffset, rotation);

	float macro = 0.5f + 0.5f * tf3d_terrain_fbm2(p, 0.62f, 6, 2.0f, 0.5f);
	float plateauWidth = clamp(u_PlateauWidth, 0.05f, 0.95f);
	float threshold = mix(0.82f, 0.46f, plateauWidth);
	float edge = mix(0.008f, 0.20f, clamp(u_EdgeSoftness, 0.001f, 1.0f));
	float mesaMask = tf3d_shape_smoothstep(threshold - edge, threshold + edge, macro);

	float base = 0.5f + 0.5f * tf3d_terrain_fbm2(p + vec2(7.0f, 23.0f), 0.45f, 4, 2.0f, 0.5f);
	float topVariation = 0.5f + 0.5f * tf3d_terrain_fbm2(p + vec2(-13.0f, 5.0f), 1.55f, 4, 2.0f, 0.5f);
	float erosionField = 0.5f + 0.5f * tf3d_terrain_fbm2(p + vec2(31.0f, -17.0f), 2.4f, 4, 2.0f, 0.5f);
	float erosion = clamp(u_Erosion, 0.0f, 1.0f);
	float topHeight = mix(0.94f + 0.06f * topVariation, 0.68f + 0.24f * erosionField, erosion);
	float height = base * 0.16f + mesaMask * topHeight * 0.84f;
	return clamp(height, 0.0f, 1.0f) * clamp(u_Strength, 0.0f, 4.0f);
}

