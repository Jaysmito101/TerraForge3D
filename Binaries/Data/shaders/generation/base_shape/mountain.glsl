{
	"Name": "Mountain",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.3,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.33,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},    
		{
			"Name": "Levels",
			"Type": "Int",
			"Default": 12,
			"Widget": "Slider",
			"Constraints": [1.0, 32.0, 0.0, 0.0]
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
			"Name": "DampingFactor",
			"Type": "Float",
			"Label": "Damping Factor",
			"Default": 0.335,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
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

float rnoise(vec2 uv)
{
	float ns = abs(tf3d_snoise2(uv));
	return ns * -2.0f + 1.0f;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	vec2 p = uv * u_Scale + u_Offset;
	float ns = 0.0f, fr = 1.0f, amp = 1.0f;
	for(int i = 0 ; i < u_Levels ; i++)
	{
		ns += rnoise(p) * amp;
		p = m * p;
		amp = amp * u_DampingFactor;
	}
	return ns * u_Strength;
}