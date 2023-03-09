#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform int u_Resolution;
uniform float u_Height;

layout(std430, binding = 0) buffer DataBuffer
{
    float data[];
};

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	data[offset] = u_Height;
}
