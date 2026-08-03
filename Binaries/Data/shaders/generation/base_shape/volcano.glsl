{
	"Name": "Volcano",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 1.5,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},    
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Constraints": [0.05, 1.0, 0.0, 0.0]
		},
		{
			"Name": "MountainFalloff",
			"Type": "Float",
			"Default": 0.2,
			"Label": "Mountain Falloff",
			"Widget": "Slider",
			"Constraints": [0.01, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Distortion",
			"Type": "Float",
			"Default": 0.1,
			"Widget": "Slider",
			"Constraints": [0.0, 0.5, 0.0, 0.0]
		},
		{
			"Name": "DistortionScale",
			"Type": "Float",
			"Default": 1.0,
			"Label": "Distortion Scale",
			"Widget": "Slider",
			"Constraints": [0.001, 8.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 12,
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
			"Name": "OutsideNoise",
			"Type": "Float",
			"Default": 0.2,
			"Label": "Outside Noise",
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "OutsideNoiseScale",
			"Type": "Float",
			"Default": 5.0,
			"Label": "Outside Noise Scale",
			"Widget": "Slider",
			"Constraints": [0.001, 16.0, 0.0, 0.0]
		},
		{
			"Name": "CraterRadius",
			"Type": "Float",
			"Default": 1.0,
			"Label": "Crater Radius",
			"Widget": "Slider",
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "CraterDepth",
			"Type": "Float",
			"Default": 2.0,
			"Label": "Crater Depth",
			"Widget": "Slider",
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		}
	]
}
// CODE
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
