#version 430 core

// work group size
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D DataSourceTexture;
layout(TF3D_FIELD_FORMAT, binding = 1) uniform image2D DataTargetTexture;

// uniforms
uniform int u_Resolution;
uniform int u_Mode;
uniform float u_Strength;
uniform bool u_UseBiomeMask;
uniform sampler2D u_BiomeMask;

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

void main(void)
{
	uvec2 offsetv2 = gl_GlobalInvocationID.xy;

	if (offsetv2.x >= u_Resolution || offsetv2.y >= u_Resolution) return;


	ivec2 pixelCoord = ivec2(offsetv2);


	if (u_Mode == 0) 
	{
		imageStore(DataTargetTexture, pixelCoord, vec4(0.0f));
	}
	else if (u_Mode == 1)
	{
		float factor = 1.0f;
		if (u_UseBiomeMask)
		{
			factor = texelFetch(u_BiomeMask, ivec2(offsetv2), 0).r;
		}

		float current = imageLoad(DataTargetTexture, pixelCoord).r;
		float source = imageLoad(DataSourceTexture, pixelCoord).r;
		imageStore(DataTargetTexture, pixelCoord, vec4(current + u_Strength * source * factor, 0.0, 0.0, 0.0));
	}
}

