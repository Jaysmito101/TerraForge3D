#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D u_Heightmap;
layout(binding = 1, rg32f) writeonly uniform image2D u_Slope;

uniform int u_Resolution;
uniform float u_SampleRadius;

float SampleHeight(ivec2 coordinate)
{
	ivec2 clampedCoordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return imageLoad(u_Heightmap, clampedCoordinate).r;
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	const int radius = max(int(round(u_SampleRadius)), 1);
	const float texel = 1.0 / max(float(u_Resolution - 1), 1.0);
	const float sampleSpacing = float(radius) * texel;

	ivec2 x = ivec2(radius, 0);
	ivec2 y = ivec2(0, radius);

	float h00 = SampleHeight(coordinate - x - y);
	float h10 = SampleHeight(coordinate      - y);
	float h20 = SampleHeight(coordinate + x - y);
	float h01 = SampleHeight(coordinate - x);
	float h21 = SampleHeight(coordinate + x);
	float h02 = SampleHeight(coordinate - x + y);
	float h12 = SampleHeight(coordinate      + y);
	float h22 = SampleHeight(coordinate + x + y);

	float gradientX = (3.0 * (h20 - h00) + 10.0 * (h21 - h01) + 3.0 * (h22 - h02)) /
		(32.0 * sampleSpacing);
	float gradientY = (3.0 * (h02 - h00) + 10.0 * (h12 - h10) + 3.0 * (h22 - h20)) /
		(32.0 * sampleSpacing);

	imageStore(u_Slope, coordinate, vec4(gradientX, gradientY, 0.0, 0.0));
}
