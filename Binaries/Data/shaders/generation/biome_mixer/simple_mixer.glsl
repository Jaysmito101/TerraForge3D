#version 430 core

// work group size
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout(std430, binding = 0) buffer DataSourceBuffer
{
	float dataSource[];
};

layout(std430, binding = 1) buffer DataTargetBuffer
{
	float dataTarget[];
};

// uniforms
uniform int u_Resolution;
uniform int u_Mode;
uniform float u_Strength;

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;

	if (offsetv2.x >= u_Resolution || offsetv2.y >= u_Resolution) return;


	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);


	if (u_Mode == 0) 
	{
		dataTarget[offset] = 0.0f;
	}
	else if (u_Mode == 1)
	{
		dataTarget[offset] = dataTarget[offset] + u_Strength * dataSource[offset];
	}
}

