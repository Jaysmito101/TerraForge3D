#version 430 core

// work group size
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// output data buffer
layout(TF3D_FIELD_FORMAT, binding = 0) uniform image2D DataTexture;

// output texture
layout(rgba32f, binding = 1) uniform image2D VisualizerTexture;

// uniforms
uniform int u_Mode;
uniform int u_Resolution;
uniform sampler2D u_DEMTexture;
uniform vec4 u_RegionToUpdate;
uniform float u_RegionTileSize;
uniform ivec2 u_DispatchOffset;
uniform float u_ZoomOnMap;
uniform float u_MapStrength;

// utility functions

// from https://documentation.maptiler.com/hc/en-us/articles/4405444055313-RGB-Terrain-by-MapTiler
float terrainRGBToElevation(vec3 res)
{
	vec3 rgb = floor(clamp(res, vec3(0.0f), vec3(1.0f)) * 255.0f + vec3(0.5f));
	return (rgb.r * 256.0f * 256.0f + rgb.g * 256.0f + rgb.b) * 0.1f - 10000.0f;
}

float sampleTerrainElevation(vec2 uv)
{
	ivec2 textureDimensions = textureSize(u_DEMTexture, 0);
	vec2 texelCoordinate = clamp(uv, vec2(0.0f), vec2(1.0f)) * vec2(textureDimensions) - vec2(0.5f);
	ivec2 p00 = ivec2(floor(texelCoordinate));
	ivec2 p11 = p00 + ivec2(1);
	vec2 blend = clamp(fract(texelCoordinate), vec2(0.0f), vec2(1.0f));
	p00 = clamp(p00, ivec2(0), textureDimensions - ivec2(1));
	p11 = clamp(p11, ivec2(0), textureDimensions - ivec2(1));

	float e00 = terrainRGBToElevation(texelFetch(u_DEMTexture, p00, 0).rgb);
	float e10 = terrainRGBToElevation(texelFetch(u_DEMTexture, ivec2(p11.x, p00.y), 0).rgb);
	float e01 = terrainRGBToElevation(texelFetch(u_DEMTexture, ivec2(p00.x, p11.y), 0).rgb);
	float e11 = terrainRGBToElevation(texelFetch(u_DEMTexture, p11, 0).rgb);
	return mix(mix(e00, e10, blend.x), mix(e01, e11, blend.x), blend.y);
}

vec2 evaluateDEM(vec2 uv)
{
	//uv *= u_ZoomOnMap;

	 // filter out un neede area
	if (uv.x < u_RegionToUpdate.x || uv.y < u_RegionToUpdate.y || uv.x >= u_RegionToUpdate.z || uv.y >= u_RegionToUpdate.w)
		return vec2(0.0f, 0.0f);

	// calculate uv for texture load
	vec2 uv2 = (uv - u_RegionToUpdate.xy) / u_RegionTileSize;
	
	float result = 0.0f;
	result = sampleTerrainElevation(uv2) * 0.00001f * clamp(u_MapStrength, 0.0f, 1000.0f);
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
		if (offsetv2.x >= uint(u_Resolution) || offsetv2.y >= uint(u_Resolution)) return;
		uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
		imageStore(DataTexture, ivec2(offsetv2), vec4(0.0f));
	}
	else if (u_Mode == 1) // the generation
	{
		uvec2 offsetv2 = uvec2(u_DispatchOffset) + gl_GlobalInvocationID.xy;
		if (offsetv2.x >= uint(u_Resolution) || offsetv2.y >= uint(u_Resolution)) return;
		uint offset = PixelCoordToDataOffset(offsetv2.x, offsetv2.y);
		vec2 uv = (vec2(offsetv2) + vec2(0.5f)) / float(u_Resolution);
		uv = vec2(uv.x, 1.0f - uv.y);
		vec2 rData = evaluateDEM(uv);
		if(rData.y > 0.5f) imageStore(DataTexture, ivec2(offsetv2), vec4(rData.x, 0.0, 0.0, 0.0));
	}
	else if (u_Mode == 2)
	{
		ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
		ivec2 visualizerSize = imageSize(VisualizerTexture);
		if (pixelCoord.x >= visualizerSize.x || pixelCoord.y >= visualizerSize.y || u_Resolution < 3) return;
		vec2 uv = (vec2(pixelCoord) + vec2(0.5f)) / vec2(imageSize(VisualizerTexture));
		uv = vec2(uv.x, 1.0f - uv.y);
		ivec2 offsetv2 = clamp(ivec2(uv * float(u_Resolution)), ivec2(1), ivec2(u_Resolution - 2));
		// Get neighboring height values
		float heightLeft = imageLoad(DataTexture, ivec2(offsetv2.x - 1, offsetv2.y)).r;
		float heightRight = imageLoad(DataTexture, ivec2(offsetv2.x + 1, offsetv2.y)).r;
		float heightTop = imageLoad(DataTexture, ivec2(offsetv2.x, offsetv2.y + 1)).r;
		float heightBottom = imageLoad(DataTexture, ivec2(offsetv2.x, offsetv2.y - 1)).r;

		// Calculate slope using central differences
		vec2 slope = (vec2(heightLeft - heightRight, heightBottom - heightTop)) * 50.0f;
		float hillShade = dot(normalize(vec3(15, 6, 15)), normalize(vec3(slope.x, 1, slope.y)));

		vec4 color = vec4(vec3(hillShade), 1.0f);
		imageStore(VisualizerTexture, pixelCoord, color);
	}
}
