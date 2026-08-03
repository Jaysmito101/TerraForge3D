{
	"Name": "Volcano",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 1.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},    
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Constraints": [0.2, 1.0, 0.0, 0.0]
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
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "DistortionScale",
			"Type": "Float",
			"Default": 1.0,
			"Label": "Distortion Scale",
			"Widget": "Slider",
			"Constraints": [0.0, 8.0, 0.0, 0.0]
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
			"Sensitivity": 0.01
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
			"Constraints": [0.0, 16.0, 0.0, 0.0]
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
	uv = uv * 2.0f - vec2(1.0) + u_Offset;
	float distortion = noise(uv * u_DistortionScale) * u_Distortion;
	float r = length(uv) + distortion;
	float craterRadius = u_Radius * u_CraterRadius * 0.3f;
	float ns = u_Height * smoothstep(u_Radius + u_MountainFalloff, craterRadius * 0.5f, r) - u_Height * u_CraterDepth * 0.3f * smoothstep(craterRadius, 0.0f, r);
	ns += tf3d_snoise2(uv * u_OutsideNoiseScale) * 0.2f * u_OutsideNoise * smoothstep(0.0, craterRadius, r);
	return ns * u_Strength;
}