#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"


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

	float beachNoise = 0.5f + 0.5f * tf3d_noise2_fbm(
		uv * terrainScale * 0.1f, u_NoiseAlgorithm, u_NoiseScale,
		terrainSeed + u_NoiseSeed, u_NoiseOctaves, u_NoiseLacunarity,
		u_NoisePersistence, u_NoiseWarp, u_NoiseJitter) * beachMask;
	float forestNoise = 0.5f + 0.5f * tf3d_noise2_fbm(
		uv * terrainScale * 0.12f, u_NoiseAlgorithm, u_NoiseScale,
		terrainSeed + 17.0f + u_NoiseSeed, u_NoiseOctaves, u_NoiseLacunarity,
		u_NoisePersistence, u_NoiseWarp, u_NoiseJitter) * forestMask;
	float mountainNoise = 0.5f + 0.5f * tf3d_noise2_fbm(
		uv * terrainScale * 0.14f, u_NoiseAlgorithm, u_NoiseScale,
		terrainSeed + 31.0f + u_NoiseSeed, u_NoiseOctaves, u_NoiseLacunarity,
		u_NoisePersistence, u_NoiseWarp, u_NoiseJitter) * mountainMask;

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
