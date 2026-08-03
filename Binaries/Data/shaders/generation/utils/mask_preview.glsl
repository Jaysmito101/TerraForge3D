#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer TerrainData
{
	float data0[];
};

layout(binding = 1, r16) uniform image2D u_MaskTexture;

uniform int u_Resolution;
uniform int u_Mode;
// x = minimum, y = maximum, z = edge softness, w = tile size
uniform vec4 u_Range;

int PixelCoordToDataOffset(ivec2 coordinate)
{
	return coordinate.y * u_Resolution + coordinate.x;
}

float TerrainValue(ivec2 coordinate)
{
	coordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return data0[PixelCoordToDataOffset(coordinate)];
}

float RangeMask(float value)
{
	float softness = max(u_Range.z, 0.000001);
	float lower = smoothstep(u_Range.x - softness, u_Range.x + softness, value);
	float upper = 1.0 - smoothstep(u_Range.y - softness, u_Range.y + softness, value);
	return clamp(lower * upper, 0.0, 1.0);
}

float SlopeDegrees(ivec2 coordinate)
{
	float top = TerrainValue(coordinate + ivec2(0, -1));
	float bottom = TerrainValue(coordinate + ivec2(0, 1));
	float left = TerrainValue(coordinate + ivec2(-1, 0));
	float right = TerrainValue(coordinate + ivec2(1, 0));
	float scale = max(u_Range.w, 0.000001);
	vec2 gradient = vec2((right - left), (bottom - top)) / (2.0 * scale);
	return degrees(atan(length(gradient)));
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float value = u_Mode == 1 ? SlopeDegrees(coordinate) : TerrainValue(coordinate);
	imageStore(u_MaskTexture, coordinate, vec4(RangeMask(value), 0.0, 0.0, 1.0));
}
