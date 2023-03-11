#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform float u_Radius;

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	// TODO
}
