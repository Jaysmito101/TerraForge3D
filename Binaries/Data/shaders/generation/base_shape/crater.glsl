{
	"Name": "Crater",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
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
			"Constraints": [0.0, 1.0, 0.0, 0.0]
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
			"Constraints": [0.00, 1.0, 0.0, 0.0]
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
	float rotation = 3.141f * u_Rotation / 180.0f;
	uv = 2.0f * uv - vec2(1.0f);
	uv = rotate(uv, rotation);
	float r = length(uv - u_Position) - tf3d_snoise2(uv) * u_LargeDistortion * 0.2f - noise(uv) * u_SmallDistortion;;
	float rad = u_Radius;
	float ns0 = u_Height * smoothstep(rad * (1 + u_OuterFalloff), rad, r) - u_Depth * smoothstep(rad, rad * u_InnerFalloff, r);
	float ns1 = tf3d_snoise2(uv) * u_LargeNoise * 0.2f + noise(uv) * u_SmallNoise;
	float ns2 = ns1 * ( u_InsideNoise * smoothstep(rad * 1.1f, rad * 0.9f, r) + u_OutsideNoise * smoothstep(rad * 0.9f, rad * 1.1f, r) ); 
	return (ns0 + ns2) * u_Strength;
}