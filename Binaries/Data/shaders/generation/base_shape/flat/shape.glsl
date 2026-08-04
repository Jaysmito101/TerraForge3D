#define SUB_STYLE_FLAT		0
#define SUB_STYLE_DOME		1
#define SUB_STYLE_SLOPE		2
#define SUB_STYLE_SINE		3

#define SIN_WAVE_TYPE_X 	0
#define SIN_WAVE_TYPE_Y 	1
#define SIN_WAVE_TYPE_XPY	2
#define SIN_WAVE_TYPE_XMY	3

#include "common/base_shape_helpers.glsl"


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
	int subStyle = clamp(u_SubStyle, SUB_STYLE_FLAT, SUB_STYLE_SINE);
	float height = clamp(u_Height, -4.0f, 4.0f);
	if(subStyle == SUB_STYLE_FLAT)
	{
		return height;
	}
	else if(subStyle == SUB_STYLE_DOME)
	{
		float radius = tf3d_shape_positive(u_Radius, 0.01f);
		float radialDistance = dot(seed, seed);
		return height * exp(-radialDistance / radius);
	}
	else if(subStyle == SUB_STYLE_SLOPE)
	{
		vec2 pos = rotate(seed.xy, clamp(u_Rotation, -36000.0f, 36000.0f));
		return pos.x * height;
	}
	else if(subStyle == SUB_STYLE_SINE)
	{
		vec2 pos = rotate(seed.xy, clamp(u_Rotation, -36000.0f, 36000.0f))
			* clamp(u_Frequency, -64.0f, 64.0f) + clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
		int waveType = clamp(u_SinWaveType, SIN_WAVE_TYPE_X, SIN_WAVE_TYPE_XMY);
		if(waveType == SIN_WAVE_TYPE_X)
		{
			return sin(pos.x) * height;
		}
		else if(waveType == SIN_WAVE_TYPE_Y)
		{
			return sin(pos.y) * height;
		}
		else if(waveType == SIN_WAVE_TYPE_XPY)
		{
			return (sin(pos.x) + sin(pos.y)) * height;
		}
		else if(waveType == SIN_WAVE_TYPE_XMY)
		{
			return (sin(pos.x) * sin(pos.y)) * height;
		}
	}
	return 0.0f;
}

