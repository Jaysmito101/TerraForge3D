#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, rg32f) writeonly uniform image2D u_Output;

uniform sampler2D u_Heightmap;
uniform sampler2D u_Pyramid;
uniform bool u_SourceIsHeightmap;
uniform int u_SourceLevel;
uniform vec2 u_SourceSize;
uniform vec2 u_OutputSize;

void main()
{
	ivec2 outputCoordinate = ivec2(gl_GlobalInvocationID.xy);
	if (float(outputCoordinate.x) >= u_OutputSize.x || float(outputCoordinate.y) >= u_OutputSize.y) return;

	ivec2 sourceSize = ivec2(u_SourceSize);
	ivec2 sourceOrigin = u_SourceIsHeightmap ? outputCoordinate : outputCoordinate * 2;
	float minimumHeight = 3.402823466e+38;
	float maximumHeight = -3.402823466e+38;

	int sampleCount = u_SourceIsHeightmap ? 1 : 4;
	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
	{
		ivec2 offset = u_SourceIsHeightmap
			? ivec2(0)
			: ivec2(sampleIndex & 1, sampleIndex >> 1);
		ivec2 sourceCoordinate = clamp(sourceOrigin + offset, ivec2(0), sourceSize - 1);
		if (u_SourceIsHeightmap)
		{
			float height = texelFetch(u_Heightmap, sourceCoordinate, 0).r;
			minimumHeight = min(minimumHeight, height);
			maximumHeight = max(maximumHeight, height);
		}
		else
		{
			vec2 bounds = texelFetch(u_Pyramid, sourceCoordinate, u_SourceLevel).rg;
			minimumHeight = min(minimumHeight, bounds.x);
			maximumHeight = max(maximumHeight, bounds.y);
		}
	}

	imageStore(u_Output, outputCoordinate, vec4(minimumHeight, maximumHeight, 0.0, 1.0));
}
