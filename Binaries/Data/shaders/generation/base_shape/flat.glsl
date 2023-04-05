{
	"Name": "Basic",
	"Params": [
		{
			"Name": "SubStyle",
			"Type": "Int",
			"Default": 0,
			"Widget": "Dropdown",
			"Options": ["Flat", "Dome", "Slope", "Sine Wave"]
		},
		{
			"Name": "SinWaveType",
			"Type": "Int",
			"Default": 0,
			"Widget": "Dropdown",
			"Options": ["X", "Y", "X + Y", "X * Y"],
			"Conditional": "SubStyle",
			"ConditionalValue": 3
		},		
		{
			"Name": "Frequency",
			"Type": "Float",
			"Default": 1.0,
			"Conditional": "SubStyle",
			"ConditionalValue": 3,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.5,
			"Conditional": "SubStyle",
			"ConditionalValue": 1,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Rotation",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Constraints": [-180.0, 180.0, 0.0, 0.0]
		},
		{
			"Name": "Offset",
			"Type": "Vector2",
			"Default": [0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01
		}
	]
}
// CODE

#define SUB_STYLE_FLAT		0
#define SUB_STYLE_DOME		1
#define SUB_STYLE_SLOPE		2
#define SUB_STYLE_SINE		3

#define SIN_WAVE_TYPE_X 	0
#define SIN_WAVE_TYPE_Y 	1
#define SIN_WAVE_TYPE_XPY	2
#define SIN_WAVE_TYPE_XMY	3


vec2 rotate(vec2 v, float a) 
{
	a = 3.141f * a / 180.0f;
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	if(u_SubStyle == SUB_STYLE_FLAT)
	{
		return u_Height;
	}
	else if(u_SubStyle == SUB_STYLE_DOME)
	{
		return u_Height * exp(-(seed.x * seed.x + seed.y * seed.y + seed.z * seed.z) / u_Radius);
	}
	else if(u_SubStyle == SUB_STYLE_SLOPE)
	{
		vec2 pos = rotate(seed.xy, u_Rotation);
		return pos.x * u_Height;
	}
	else if(u_SubStyle == SUB_STYLE_SINE)
	{
		vec2 pos = rotate(seed.xy, u_Rotation) * u_Frequency + u_Offset;
		if(u_SinWaveType == SIN_WAVE_TYPE_X)
		{
			return sin(pos.x) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_Y)
		{
			return sin(pos.y) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_XPY)
		{
			return (sin(pos.x) + sin(pos.y)) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_XMY)
		{
			return (sin(pos.x) * sin(pos.y)) * u_Height;
		}
	}
	return 0.0f;
}
