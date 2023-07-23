#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout (std430, binding = 0) buffer DataSourceBuffer
{
	float dataSource[];
};

layout(rgba32f, binding = 1) uniform image2D VisualizerTexture;


// uniforms
uniform int u_Resolution;
uniform vec2 u_HmapMinMax;


uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}


void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	vec2 uv = vec2(offsetv2) / float(u_Resolution);
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	float value = dataSource[offset];
	value = (value - u_HmapMinMax.x) / (u_HmapMinMax.y - u_HmapMinMax.x);
	offsetv2 = uvec2(uv * vec2(256));
	imageStore(VisualizerTexture, ivec2(offsetv2), vec4(vec3(value), 1.0f));
}

