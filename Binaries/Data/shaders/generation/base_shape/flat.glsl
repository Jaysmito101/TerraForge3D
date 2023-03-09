#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#define SUB_STYLE_FLAT		0
#define SUB_STYLE_DOME		1
#define SUB_STYLE_SLOPE		2
#define SUB_STYLE_SINE		3
#define SIN_WAVE_TYPE_X 	0
#define SIN_WAVE_TYPE_Y 	1
#define SIN_WAVE_TYPE_XPY	2
#define SIN_WAVE_TYPE_XMY	3

uniform int u_Resolution;
uniform vec2 u_Frequency;
uniform vec2 u_Offset;
uniform int u_SubStyle;
uniform int u_SinWaveType;
uniform float u_Height;
uniform float u_Radius;
uniform float u_Rotation;


layout(std430, binding = 0) buffer DataBuffer
{
    float data[];
};

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

vec2 rotate(vec2 v, float a) {
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	vec2 uv = offsetv2 / float(u_Resolution);
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	if(u_SubStyle == SUB_STYLE_FLAT)
	{
		data[offset] = u_Height;		
	}
	else if(u_SubStyle == SUB_STYLE_DOME)
	{
		data[offset] = u_Height;
	}
	else if(u_SubStyle == SUB_STYLE_SLOPE)
	{
		vec2 pos = rotate(uv, u_Rotation);
		data[offset] = pos.x * u_Height;
	}
	else if(u_SubStyle == SUB_STYLE_SINE)
	{
		vec2 pos = rotate(uv, u_Rotation) * u_Frequency + u_Offset;
		if(u_SinWaveType == SIN_WAVE_TYPE_X)
		{
			data[offset] = sin(pos.x) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_Y)
		{
			data[offset] = sin(pos.y) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_XPY)
		{
			data[offset] = (sin(pos.x) + sin(pos.y)) * u_Height;
		}
		else if(u_SinWaveType == SIN_WAVE_TYPE_XMY)
		{
			data[offset] = (sin(pos.x) * sin(pos.y)) * u_Height;
		}
	}
}
