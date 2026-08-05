#ifndef TF3D_HEIGHTFIELD_PYRAMID_SAMPLING_GLSL
#define TF3D_HEIGHTFIELD_PYRAMID_SAMPLING_GLSL

const float TF3D_HEIGHTFIELD_SAMPLING_EPSILON = 0.000001;

float TF3D_SamplePyramidChannel(sampler2D pyramid, vec2 uv, int level, int channel)
{
	ivec2 levelSize = textureSize(pyramid, level);
	vec2 samplePosition = clamp(uv, vec2(0.0), vec2(1.0)) * vec2(levelSize - ivec2(1));
	ivec2 lower = ivec2(floor(samplePosition));
	ivec2 upper = min(lower + ivec2(1), levelSize - ivec2(1));
	vec2 blend = fract(samplePosition);

	float lowerLeft = texelFetch(pyramid, lower, level)[channel];
	float lowerRight = texelFetch(pyramid, ivec2(upper.x, lower.y), level)[channel];
	float upperLeft = texelFetch(pyramid, ivec2(lower.x, upper.y), level)[channel];
	float upperRight = texelFetch(pyramid, upper, level)[channel];
	return mix(mix(lowerLeft, lowerRight, blend.x), mix(upperLeft, upperRight, blend.x), blend.y);
}

vec3 TF3D_SamplePyramidTerrainNormal(sampler2D pyramid, vec2 uv, vec2 terrainWorldSize)
{
	ivec2 pyramidSize = textureSize(pyramid, 0);
	vec2 texel = 1.0 / vec2(max(pyramidSize.x - 1, 1), max(pyramidSize.y - 1, 1));
	float leftHeight = TF3D_SamplePyramidChannel(pyramid, uv - vec2(texel.x, 0.0), 0, 2);
	float rightHeight = TF3D_SamplePyramidChannel(pyramid, uv + vec2(texel.x, 0.0), 0, 2);
	float lowerHeight = TF3D_SamplePyramidChannel(pyramid, uv - vec2(0.0, texel.y), 0, 2);
	float upperHeight = TF3D_SamplePyramidChannel(pyramid, uv + vec2(0.0, texel.y), 0, 2);
	vec2 gradient = vec2(
		(rightHeight - leftHeight) / max(2.0 * texel.x, TF3D_HEIGHTFIELD_SAMPLING_EPSILON),
		(upperHeight - lowerHeight) / max(2.0 * texel.y, TF3D_HEIGHTFIELD_SAMPLING_EPSILON));
	return normalize(vec3(-gradient.x / terrainWorldSize.x, 1.0,
		gradient.y / terrainWorldSize.y));
}

vec2 TF3D_OctahedralEncode(vec3 normal)
{
	normal /= max(abs(normal.x) + abs(normal.y) + abs(normal.z), TF3D_HEIGHTFIELD_SAMPLING_EPSILON);
	vec2 encoded = normal.xz;
	if (normal.y < 0.0)
	{
		encoded = (1.0 - abs(encoded.yx)) * sign(encoded);
	}
	return encoded * 0.5 + 0.5;
}

vec3 TF3D_OctahedralDecode(vec2 encoded)
{
	vec2 folded = encoded * 2.0 - 1.0;
	vec3 normal = vec3(folded.x, 1.0 - abs(folded.x) - abs(folded.y), folded.y);
	if (normal.y < 0.0)
	{
		normal.xz = (1.0 - abs(normal.zx)) * sign(normal.xz);
	}
	return normalize(normal);
}

#endif
