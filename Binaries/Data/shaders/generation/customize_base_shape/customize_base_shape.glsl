#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D DataSourceTexture;
layout(TF3D_FIELD_FORMAT, binding = 1) writeonly uniform image2D DataTargetTexture;
layout(binding = 2) uniform sampler2D u_MaskTexture;

uniform int u_Resolution;
uniform int u_UseMask;
uniform int u_FlattenSource;
uniform int u_Direction;
uniform float u_Strength;
uniform float u_Smoothing;

float sampleMask(ivec2 coordinate)
{
	ivec2 clampedCoordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return texelFetch(u_MaskTexture, clampedCoordinate, 0).r;
}

float smoothedMask(ivec2 coordinate)
{
	float value = 0.0f;
	float weight = 0.0f;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			float sampleWeight = (x == 0 && y == 0) ? 4.0f : (x == 0 || y == 0 ? 2.0f : 1.0f);
			value += sampleMask(coordinate + ivec2(x, y)) * sampleWeight;
			weight += sampleWeight;
		}
	}
	return value / max(weight, 0.000001f);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) {
		return;
	}

	float value = u_FlattenSource != 0 ? 0.0f : imageLoad(DataSourceTexture, coordinate).r;
	if (u_UseMask != 0)
	{
		float mask = sampleMask(coordinate);
		mask = mix(mask, smoothedMask(coordinate), clamp(u_Smoothing, 0.0f, 1.0f));
		value += float(u_Direction) * mask * u_Strength;
	}

	imageStore(DataTargetTexture, coordinate, vec4(value, 0.0f, 0.0f, 0.0f));
}
