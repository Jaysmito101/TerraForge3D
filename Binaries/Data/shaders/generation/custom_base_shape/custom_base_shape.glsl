#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout (std430, binding = 0) buffer DataSourceBuffer
{
	float dataSource[];
};

layout (std430, binding = 1) buffer DataTargetBuffer
{
	float dataTarget[];
};

// uniforms
uniform int u_Mode;
uniform int u_Resolution;

uniform vec2 u_BrushPosition;
uniform vec4 u_BrushSettings0;
uniform float u_MixFactor;


uint PixelCoordToDataOffset2(uint x, uint y, uint res)
{
	return y * res + x;
}

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}


void transferData()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);
	dataTarget[offset] = dataSource[offset] * u_MixFactor;
}

float calculateFallOff(in vec2 uv)
{
	float distanceVal = length(uv - u_BrushPosition);
	return smoothstep(u_BrushSettings0.y * (1.0f - u_BrushSettings0.z), u_BrushSettings0.y, distanceVal);

}

void applyBasicBrush()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);

	// u_BrushSettings0 = strength, size, falloff, reserved
	float val = mix(u_BrushSettings0.x, 0.0f, calculateFallOff(uv));
	dataTarget[offset] = dataTarget[offset] + val * u_MixFactor;
}

void applyGaussianFilter()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	ivec2 offsetiv2 = ivec2(offsetv2);
	uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
	vec2 uv = offsetv2 / float(u_Resolution);
	
	const float filterMask[5][5] = float[][](
		float[](1.0f, 4.0f, 7.0f, 4.0f, 1.0f),
		float[](4.0f, 16.0f, 26.0f, 16.0f, 4.0f),
		float[](7.0f, 26.0f, 41.0f, 26.0f, 7.0f),
		float[](4.0f, 16.0f, 26.0f, 16.0f, 4.0f),
		float[](1.0f, 4.0f, 7.0f, 4.0f, 1.0f)
	);

	float sum = 0.0f;

	for(int i = -2; i <= 2; i++)
	{
		for(int j = -2; j <= 2; j++)
		{
			ivec2 offsetiv2 = ivec2(offsetv2) + ivec2(i, j);
			if(offsetiv2.x < 0 || offsetiv2.x >= u_Resolution || offsetiv2.y < 0 || offsetiv2.y >= u_Resolution) continue;
			uint offset = PixelCoordToDataOffset(offsetiv2.x, offsetiv2.y);
			sum += dataSource[offset] * filterMask[i + 2][j + 2];
		}
	}
	dataTarget[offset] = mix(sum / 273.0f, dataTarget[offset], calculateFallOff(uv));
}

void main(void)
{
	if(u_Mode == 0) transferData();
	else if (u_Mode == 1) applyBasicBrush();
	else if (u_Mode == 2) applyGaussianFilter();
}

