{
	"Name": "Crater",
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
			"Name": "Rotation",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Constraints": [-180.00, 180.00, 0.0, 0.0]
		},    
		{
			"Name": "Position",
			"Type": "Vector2",
			"Default": [0.0, 0.0],
			"Widget": "Slider",
			"Constraints": [-1.00, 1.00, 0.0, 0.0]
		},
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.4,
			"Widget": "Slider",
			"Constraints": [0.03, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Depth",
			"Type": "Float",
			"Default": 0.634,
			"Widget": "Slider",
			"Constraints": [0.0, 2.0, 0.0, 0.0]
		},
		{
			"Name": "InnerFalloff",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Label": "Inner Falloff",
			"Constraints": [0.0, 0.999, 0.0, 0.0]
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 0.179,
			"Widget": "Slider",
			"Constraints": [0.0, 2.0, 0.0, 0.0]
		},
		{
			"Name": "OuterFalloff",
			"Type": "Float",
			"Default": 0.8,
			"Widget": "Slider",
			"Label": "Outer Falloff",
			"Constraints": [0.01, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 0,
			"Widget": "Seed"
		},
		{
			"Name": "LargeDistortion",
			"Type": "Float",
			"Default": 0.2,
			"Widget": "Slider",
			"Label": "Large Distortion",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "SmallDistortion",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Label": "Small Distortion",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "LargeNoise",
			"Type": "Float",
			"Default": 0.2,
			"Widget": "Slider",
			"Label": "Large Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "SmallNoise",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Label": "Small Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "InsideNoise",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Label": "Inside Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "OutsideNoise",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Slider",
			"Label": "Outside Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		}
	]
}
// CODE
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
		+ clamp(u_OutsideNoise, 0.0f, 1.0f) * outsideMask);
	return (ns0 + ns2) * clamp(u_Strength, 0.0f, 4.0f);
}
