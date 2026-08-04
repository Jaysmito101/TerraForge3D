#include "common/base_shape_helpers.glsl"


// Modified hash33 by Dave_Hoskins (original does not play well with simplex)
// Original Source: https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(0.1031, 0.11369, 0.13787));
    p3 += dot(p3, p3.yxz + 19.19);
    return -1.0 + 2.0 * fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

// Raw simplex implementation by candycat
// Source: https://www.shadertoy.com/view/4sc3z2
float SimplexNoiseRaw(vec3 pos)
{
    const float K1 = 0.333333333;
    const float K2 = 0.166666667;
    
    vec3 i = floor(pos + (pos.x + pos.y + pos.z) * K1);
    vec3 d0 = pos - (i - (i.x + i.y + i.z) * K2);
    
    vec3 e = step(vec3(0.0), d0 - d0.yzx);
	vec3 i1 = e * (1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    vec3 d1 = d0 - (i1 - 1.0 * K2);
    vec3 d2 = d0 - (i2 - 2.0 * K2);
    vec3 d3 = d0 - (1.0 - 3.0 * K2);
    
    vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
    vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));
    
    return dot(vec4(31.316), n);
}

float SimplexNoise(
    vec3  pos,
    int   octaves,
    float scale,
    float persistence,
    float lacunarity)
{
    float final        = 0.0;
    float amplitude    = 1.0;
    float maxAmplitude = 0.0;
    float frequency    = tf3d_shape_positive(scale, 0.001f);
    int octaveCount    = clamp(octaves, 1, 16);

    for(int i = 0; i < octaveCount; ++i)
    {
        final        += SimplexNoiseRaw(pos * frequency) * amplitude;
        maxAmplitude += amplitude;
        frequency    *= clamp(lacunarity, 1.0f, 4.0f);
        amplitude    *= clamp(persistence, 0.0f, 0.95f);
    }

    return final / max(maxAmplitude, TF3D_SHAPE_EPSILON);
}

vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.14159265f * clamp(u_Rotation, -36000.0f, 36000.0f) / 180.0f;
	uv = uv * 2.0f - vec2(1.0f) + clamp(u_Offset, vec2(-10000.0f), vec2(10000.0f));
	uv = rotate(uv, rotation);

	float beachCoverage = tf3d_shape_positive(u_BeachCoverage, 0.05f);
	float beachSteepness = clamp(u_BeachSteepness, 0.0f, 1.0f);
	float forestSteepness = clamp(u_ForestSteepness, 0.0f, 1.0f);
	float beachCoverageFactor = 1.0f - clamp(u_BeachCoverage, 0.0f, 1.0f);
	float forestCoverageFactor = beachCoverageFactor
		+ (1.0f - beachCoverageFactor) * (1.0f - clamp(u_ForestCoverage, 0.0f, 1.0f));
	float mountainCoverageFactor = forestCoverageFactor
		+ (1.0f - forestCoverageFactor) * (1.0f - clamp(u_MountainCoverage, 0.0f, 1.0f));

	float rad = length(uv) / beachCoverage;
	float baseMask = exp(-rad * rad * 2.0f);
	float beachEdge = clamp(beachCoverageFactor + 1.0f - beachSteepness * exp(-rad) * 0.9f, beachCoverageFactor, 1.0f);
	float forestEdge = clamp(forestCoverageFactor + 1.0f - forestSteepness * exp(-rad) * 0.9f, forestCoverageFactor, 1.0f);
	float mountainEdge = clamp(mountainCoverageFactor + 1.0f, mountainCoverageFactor, 1.0f);
	float beachMask = tf3d_shape_smoothstep(beachCoverageFactor, beachEdge, baseMask);
	float forestMask = tf3d_shape_smoothstep(forestCoverageFactor, forestEdge, baseMask);
	float mountainMask = tf3d_shape_smoothstep(mountainCoverageFactor, mountainEdge, baseMask);

	float terrainScale = tf3d_shape_positive(u_TerrainScale, 0.01f);
	int terrainOctaves = clamp(u_TerrainOctaves, 1, 16);
	float persistence = clamp(u_TerrainPersistence, 0.0f, 0.95f);
	float lacunarity = clamp(u_TerrainLacunarity, 1.0f, 4.0f);
	float terrainSeed = float(u_Seed);

	float beachNoise = SimplexNoise(vec3(uv * terrainScale * 0.1f, terrainSeed), terrainOctaves, 1.0f, persistence, lacunarity) * beachMask;
	float forestNoise = SimplexNoise(vec3(uv * terrainScale * 0.12f, terrainSeed + 17.0f), terrainOctaves, 1.0f, persistence, lacunarity) * forestMask;
	float mountainNoise = SimplexNoise(vec3(uv * terrainScale * 0.14f, terrainSeed + 31.0f), terrainOctaves, 1.0f, persistence, lacunarity) * mountainMask;

	float beachResult = tf3d_shape_smoothstep(beachCoverageFactor, beachEdge, beachNoise);
	float forestResult = tf3d_shape_smoothstep(forestCoverageFactor, forestEdge, forestNoise);
	float mountainResult = tf3d_shape_smoothstep(mountainCoverageFactor, mountainEdge, mountainNoise);

	float beachBand = clamp(beachResult - forestResult, 0.0f, 1.0f);
	float forestBand = clamp(forestResult - mountainResult, 0.0f, 1.0f);
	float mountainBand = clamp(mountainResult, 0.0f, 1.0f);

	float beachHeight = clamp(u_BeachHeight, 0.0f, 2.0f);
	float forestHeight = clamp(u_ForestHeight, -2.0f, 2.0f);
	float mountainHeight = clamp(u_MountainHeight, -2.0f, 2.0f);
	return clamp(beachBand * beachHeight
		+ forestBand * forestHeight
		+ mountainBand * mountainHeight, -2.0f, 2.0f);
}

