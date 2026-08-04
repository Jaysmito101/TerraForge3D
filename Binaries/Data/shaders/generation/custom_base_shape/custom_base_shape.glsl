#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D DataSourceTexture;
layout(TF3D_FIELD_FORMAT, binding = 1) uniform image2D DataTargetTexture;

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
	ivec2 pixelCoord = ivec2(offsetv2);
	vec2 uv = offsetv2 / float(u_Resolution);
	imageStore(DataTargetTexture, pixelCoord, vec4(imageLoad(DataSourceTexture, pixelCoord).r * u_MixFactor, 0.0, 0.0, 0.0));
}

float calculateFallOff(in vec2 uv)
{
	float distanceVal = length(uv - u_BrushPosition);
	return smoothstep(u_BrushSettings0.y * (1.0f - u_BrushSettings0.z), u_BrushSettings0.y, distanceVal);

}

void applyBasicBrush()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	ivec2 pixelCoord = ivec2(offsetv2);
	vec2 uv = offsetv2 / float(u_Resolution);

	// u_BrushSettings0 = strength, size, falloff, reserved
	float val = mix(u_BrushSettings0.x, 0.0f, calculateFallOff(uv));
	float current = imageLoad(DataTargetTexture, pixelCoord).r;
	imageStore(DataTargetTexture, pixelCoord, vec4(current + val * u_MixFactor, 0.0, 0.0, 0.0));
}

void applyGaussianFilter()
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;
	ivec2 offsetiv2 = ivec2(offsetv2);
	ivec2 pixelCoord = ivec2(offsetv2);
	vec2 uv = offsetv2 / float(u_Resolution);
	
	const float filterMask[5][5] = float[][](
		float[](1.0f, 4.0f, 7.0f, 4.0f, 1.0f),
		float[](4.0f, 16.0f, 26.0f, 16.0f, 4.0f),
		float[](7.0f, 26.0f, 41.0f, 26.0f, 7.0f),
		float[](4.0f, 16.0f, 26.0f, 16.0f, 4.0f),
		float[](1.0f, 4.0f, 7.0f, 4.0f, 1.0f)
	);

	float sum = 0.0f;
	float weightSum = 0.0f;

	for(int i = -2; i <= 2; i++)
	{
		for(int j = -2; j <= 2; j++)
		{
			ivec2 offsetiv2 = ivec2(offsetv2) + ivec2(i, j);
			if(offsetiv2.x < 0 || offsetiv2.x >= u_Resolution || offsetiv2.y < 0 || offsetiv2.y >= u_Resolution) continue;
			float weight = filterMask[i + 2][j + 2];
			sum += imageLoad(DataSourceTexture, offsetiv2).r * weight;
			weightSum += weight;
		}
	}
	float current = imageLoad(DataTargetTexture, pixelCoord).r;
	imageStore(DataTargetTexture, pixelCoord, vec4(mix(sum / max(weightSum, 0.000001f), current, calculateFallOff(uv)), 0.0, 0.0, 0.0));
}

void main(void)
{
	uvec2 invocation = gl_GlobalInvocationID.xy;
	if (invocation.x >= uint(u_Resolution) || invocation.y >= uint(u_Resolution)) return;
	if(u_Mode == 0) transferData();
	else if (u_Mode == 1) applyBasicBrush();
	else if (u_Mode == 2) applyGaussianFilter();
}

