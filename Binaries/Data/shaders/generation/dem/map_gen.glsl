#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout (std430, binding = 0) buffer DataBuffer
{
	float data[];
};

// output texture
layout(rgba32f, binding = 1) uniform image2D VisualizerTexture;

// uniforms
uniform int u_Mode;
uniform int u_Resolution;
uniform sampler2D u_DEMTexture;
uniform vec4 u_RegionToUpdate;
uniform float u_RegionTileSize;
uniform float u_ZoomOnMap;
uniform float u_MapStrength;

// utility functions

// from https://documentation.maptiler.com/hc/en-us/articles/4405444055313-RGB-Terrain-by-MapTiler
float terrainRGBToElevation(vec3 res)
{
	return (res.r * 256.0f * 256.0f * 256.0f + res.g * 256.0f * 256.0f + res.b * 256.0f ) * 0.1 - 10000.0f;
}

vec2 evaluateDEM(vec2 uv)
{
	//uv *= u_ZoomOnMap;

	 // filter out un neede area
	if (uv.x < u_RegionToUpdate.x || uv.y < u_RegionToUpdate.y || uv.x > u_RegionToUpdate.z || uv.y > u_RegionToUpdate.w) 
		return vec2(0.0f, 0.0f);

	// calculate uv for texture load
	vec2 uv2 = (uv - u_RegionToUpdate.xy) / u_RegionTileSize;
	
	float result = 0.0f;
	result =  terrainRGBToElevation(texture(u_DEMTexture, uv2).rgb) * 0.00001 * u_MapStrength;
	return vec2(result, 1.0f); 
}

uint PixelCoordToDataOffset(uint x, uint y)
{
	return y * u_Resolution + x;
}

void main()
{
	if(u_Mode == 0) // clear map
	{
		uvec2 offsetv2 = gl_GlobalInvocationID.xy;
		uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
		data[offset] = 0.0f;
	}
	else if (u_Mode == 1) // the generation
	{
		uvec2 offsetv2 = gl_GlobalInvocationID.xy;
		uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
		vec2 uv = offsetv2 / float(u_Resolution);
		uv = vec2(uv.x, 1.0f - uv.y);
		vec2 rData = evaluateDEM(uv);
		if(rData.y > 0.5f) data[offset] = rData.x;
	}
	else if (u_Mode == 2)
	{
		ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
		vec2 uv = pixelCoord / vec2(imageSize(VisualizerTexture));
		uv = vec2(uv.x, 1.0f - uv.y);
		uvec2 offsetv2 = uvec2(uv * u_Resolution);
		// Get neighboring height values
		float heightLeft = data[PixelCoordToDataOffset(offsetv2.x - 1, offsetv2.y)];
		float heightRight = data[PixelCoordToDataOffset(offsetv2.x + 1, offsetv2.y)];
		float heightTop = data[PixelCoordToDataOffset(offsetv2.x, offsetv2.y + 1)];
		float heightBottom = data[PixelCoordToDataOffset(offsetv2.x, offsetv2.y - 1)];

		// Calculate slope using central differences
		vec2 slope = (vec2(heightLeft - heightRight, heightBottom - heightTop)) * 50.0f;
		float hillShade = dot(normalize(vec3(15, 6, 15)), normalize(vec3(slope.x, 1, slope.y)));

		vec4 color = vec4(vec3(hillShade), 1.0f);
		imageStore(VisualizerTexture, pixelCoord, color);
	}
}
