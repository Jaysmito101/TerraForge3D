{
	"Name": "Classic",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.001
		},
		{
			"Name": "Levels",
			"Type": "Int",
			"Default": 1,
			"Widget": "Slider",
			"Constraints": [0.0, 6.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 42,
			"Widget": "Seed"
		},
		{
			"Name": "Offset",
			"Type": "Vector3",
			"Default": [0.0, 0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "SquareValue",
			"Type": "Bool",
			"Default": false,
			"Widget": "Checkbox",
			"Label": "Square Value"
		},
		{
			"Name": "AbsoluteValue",
			"Type": "Bool",
			"Default": false,
			"Widget": "Checkbox",
			"Label": "Absolute Value"
		}
	]
}
// CODE

#include "common/noise_3d.glsl"

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	seed = (seed + u_Offset) * u_Scale + vec3(u_Seed);
	float n = 0.0f;
	for(int i = 0 ; i < u_Levels ; i++)
	{
		seed = vec3(tf3d_cnoise(seed + vec3(0.0f, 0.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(1.0f, 2.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(3.0f, 4.0f, 0.0f))
					);
	}
	n = tf3d_cnoise(seed);
	if(u_AbsoluteValue) n = abs(n);
	if(u_SquareValue) n = n * n;	
	return n * u_Strength;
}