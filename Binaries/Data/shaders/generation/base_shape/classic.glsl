{
	"Name": "Classic",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [-4.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.001,
			"Constraints": [0.001, 16.0, 0.0, 0.0]
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
#include "common/base_shape_helpers.glsl"

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.001);
	vec3 offset = clamp(u_Offset, vec3(-10000.0), vec3(10000.0));
	seed = (seed + offset) * scale + vec3(u_Seed);
	float n = 0.0f;
	int levels = clamp(u_Levels, 0, 6);
	for(int i = 0 ; i < levels ; i++)
	{
		seed = vec3(tf3d_cnoise(seed + vec3(0.0f, 0.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(1.0f, 2.0f, 0.0f)),
			tf3d_cnoise(seed + vec3(3.0f, 4.0f, 0.0f))
					);
	}
	n = clamp(tf3d_cnoise(seed), -1.0f, 1.0f);
	if(u_AbsoluteValue) n = abs(n);
	if(u_SquareValue) n = n * n;
	return n * clamp(u_Strength, -4.0f, 4.0f);
}
