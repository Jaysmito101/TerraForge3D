#version 430 core

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec4 aNormal;
layout (location = 2) in vec4 aTexCoord;

out SeaVertexData
{
	vec3 worldPosition;
	vec3 waveNormal;
	vec2 texCoord;
	float waterDepth;
	float waveSteepness;
	float terrainCoverage;
} vertexOutput;

uniform mat4 u_ProjectionView;
uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform vec2 u_TerrainMinimumXZ;
uniform vec2 u_TerrainWorldSize;
uniform vec2 u_SurfaceMinimumXZ;
uniform vec2 u_SurfaceWorldSize;
uniform float u_TerrainHeightOffset;
uniform float u_SeaLevel;
uniform float u_SeaWorldHeight;
uniform float u_BottomWorldHeight;
uniform float u_SideEdgeOffset;
uniform float u_Time;

uniform float u_WaveAmplitude;
uniform float u_WaveLength;
uniform float u_WaveSpeed;
uniform float u_WaveChoppiness;
uniform float u_ShoreWidth;
uniform float u_DeepDepth;
uniform bool u_VerticalSidePass;

#include "common/heightfield_pyramid_sampling.glsl"

const float TF3D_PI = 3.141592653589793;

float TF3D_WaveHash(vec2 value)
{
	return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453123);
}

float TF3D_WaveNoise(vec2 value)
{
	vec2 cell = floor(value);
	vec2 local = fract(value);
	local = local * local * (3.0 - 2.0 * local);
	float lower = mix(TF3D_WaveHash(cell),
		TF3D_WaveHash(cell + vec2(1.0, 0.0)), local.x);
	float upper = mix(TF3D_WaveHash(cell + vec2(0.0, 1.0)),
		TF3D_WaveHash(cell + vec2(1.0, 1.0)), local.x);
	return mix(lower, upper, local.y);
}

float EvaluateWaveShape(vec2 worldXZ)
{
	vec2 drift = vec2(u_Time * u_WaveSpeed * 0.11,
		-u_Time * u_WaveSpeed * 0.073);
	float terrainScale = max(max(u_TerrainWorldSize.x, u_TerrainWorldSize.y), 0.001);
	vec2 macroCoordinate = worldXZ / terrainScale;
	vec2 macroWarp = vec2(
		TF3D_WaveNoise(macroCoordinate * 0.72 + drift * 0.18),
		TF3D_WaveNoise(macroCoordinate * 0.72 - drift * 0.13 + vec2(23.1, 8.4))) - 0.5;
	float macroShape = TF3D_WaveNoise((macroCoordinate + macroWarp * 0.85) * 1.25 + drift * 0.20);
	vec2 coarseCoordinate = worldXZ / max(u_WaveLength * terrainScale, 0.001);
	vec2 warp = vec2(
		TF3D_WaveNoise(coarseCoordinate * 0.62 + drift),
		TF3D_WaveNoise(coarseCoordinate * 0.62 - drift + vec2(17.3, 4.1))) - 0.5;
	vec2 warped = coarseCoordinate + warp * 1.35;
	float broad = TF3D_WaveNoise(warped * 0.82 + drift);
	float detail = TF3D_WaveNoise(warped * 2.45 - drift * 1.7);
	float ridges = 1.0 - abs(TF3D_WaveNoise(warped * 4.1 + drift * 0.4) * 2.0 - 1.0);
	return clamp(macroShape * 0.34 + broad * 0.40 + detail * 0.17 + ridges * 0.09, 0.0, 1.0);
}

struct WaveResult
{
	vec2 horizontalDisplacement;
	float verticalDisplacement;
	vec2 heightGradient;
};

WaveResult EvaluateWaves(vec2 worldXZ, float waterDepth)
{
	WaveResult result;
	result.horizontalDisplacement = vec2(0.0);
	result.verticalDisplacement = 0.0;
	result.heightGradient = vec2(0.0);

	const vec2 directions[6] = vec2[6](
		vec2(0.96, 0.28),
		vec2(-0.42, 0.91),
		vec2(0.68, -0.73),
		vec2(-0.88, -0.46),
		vec2(0.17, -0.985),
		vec2(-0.72, 0.54));
	const float lengthScale[6] = float[6](1.0, 0.63, 0.38, 0.22, 0.31, 0.16);
	const float amplitudeScale[6] = float[6](1.0, 0.48, 0.25, 0.12, 0.16, 0.07);
	const float speedScale[6] = float[6](1.0, 1.27, 0.78, 1.64, 0.56, 1.91);

	float shallowAttenuation = smoothstep(0.0, max(u_ShoreWidth * 2.5, 0.0001), max(waterDepth, 0.0));
	float depthSafety = clamp(waterDepth / max(u_WaveAmplitude * 1.75, 0.0001), 0.0, 1.0);
	float attenuation = shallowAttenuation * mix(0.25, 1.0, depthSafety);

	for (int index = 0; index < 6; ++index)
	{
		vec2 direction = normalize(directions[index]);
		float terrainScale = max(max(u_TerrainWorldSize.x, u_TerrainWorldSize.y), 0.001);
		float wavelength = max(u_WaveLength * terrainScale * lengthScale[index], 0.001);
		float waveNumber = 2.0 * TF3D_PI / wavelength;
		float amplitude = u_WaveAmplitude * amplitudeScale[index] * attenuation;
		float phase = dot(worldXZ, direction) * waveNumber - u_Time * u_WaveSpeed * speedScale[index];
		float sine = sin(phase);
		float cosine = cos(phase);
		float harmonicSine = sin(phase * 2.0 + 1.17);
		float harmonicCosine = cos(phase * 2.0 + 1.17);

		result.verticalDisplacement += amplitude * (sine + harmonicSine * 0.18);
		result.horizontalDisplacement += direction *
			(amplitude * u_WaveChoppiness * 0.55 * (cosine + harmonicCosine * 0.12));
		result.heightGradient += direction * (amplitude * waveNumber *
			(cosine + harmonicCosine * 0.36));
	}

	float terrainScale = max(max(u_TerrainWorldSize.x, u_TerrainWorldSize.y), 0.001);
	float shapeEpsilon = max(u_WaveLength * terrainScale * 0.035, 0.002);
	float shapeCenter = EvaluateWaveShape(worldXZ);
	float shapeX = EvaluateWaveShape(worldXZ + vec2(shapeEpsilon, 0.0));
	float shapeZ = EvaluateWaveShape(worldXZ + vec2(0.0, shapeEpsilon));
	float shapeAmplitude = u_WaveAmplitude * attenuation * 0.42;
	result.verticalDisplacement += (shapeCenter - 0.5) * shapeAmplitude;
	result.heightGradient += vec2(
		(shapeX - shapeCenter) / shapeEpsilon,
		(shapeZ - shapeCenter) / shapeEpsilon) * shapeAmplitude;

	return result;
}

void main()
{
	vec2 texCoord = aTexCoord.xy;
	if (u_VerticalSidePass)
	{
		vec2 worldXZ = u_TerrainMinimumXZ + vec2(
			texCoord.x * u_TerrainWorldSize.x,
			(1.0 - texCoord.y) * u_TerrainWorldSize.y);
		worldXZ += aNormal.xz * u_SideEdgeOffset;
		vec3 position = vec3(worldXZ.x,
			mix(u_BottomWorldHeight, u_SeaWorldHeight, aPosition.y), worldXZ.y);
		float edgeTerrainHeight = TF3D_SamplePyramidChannel(
			u_HeightPyramid, texCoord, 0, 2) + u_TerrainHeightOffset;
		vertexOutput.worldPosition = position;
		vertexOutput.waveNormal = normalize(aNormal.xyz);
		vertexOutput.texCoord = texCoord;
		vertexOutput.waterDepth = u_SeaWorldHeight - edgeTerrainHeight;
		vertexOutput.waveSteepness = 0.0;
		vertexOutput.terrainCoverage = 0.0;
		gl_Position = u_ProjectionView * vec4(position, 1.0);
		return;
	}

	vec2 worldXZ = u_SurfaceMinimumXZ + vec2(
		texCoord.x * u_SurfaceWorldSize.x,
		(1.0 - texCoord.y) * u_SurfaceWorldSize.y);
	vec2 rawTerrainTexCoord = vec2(
		(worldXZ.x - u_TerrainMinimumXZ.x) / max(u_TerrainWorldSize.x, 0.0001),
		(u_TerrainMinimumXZ.y + u_TerrainWorldSize.y - worldXZ.y) / max(u_TerrainWorldSize.y, 0.0001));
	float terrainCoverage = (rawTerrainTexCoord.x >= 0.0 && rawTerrainTexCoord.x <= 1.0 &&
		rawTerrainTexCoord.y >= 0.0 && rawTerrainTexCoord.y <= 1.0) ? 1.0 : 0.0;
	vec2 terrainTexCoord = clamp(rawTerrainTexCoord, vec2(0.0), vec2(1.0));

	float terrainHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, terrainTexCoord, 0, 2) + u_TerrainHeightOffset;
	float sampledWaterDepth = u_SeaWorldHeight - terrainHeight;
	float waterDepth = terrainCoverage > 0.5 ? sampledWaterDepth : max(sampledWaterDepth, u_DeepDepth * 2.0);
	WaveResult waves = EvaluateWaves(worldXZ, waterDepth);
	float edgeDistance = min(min(texCoord.x, 1.0 - texCoord.x),
		min(texCoord.y, 1.0 - texCoord.y));
	float edgeWaveFade = smoothstep(0.0, 0.04, edgeDistance);
	waves.horizontalDisplacement *= edgeWaveFade;
	waves.verticalDisplacement *= edgeWaveFade;
	waves.heightGradient *= edgeWaveFade;

	vec3 position = vec3(worldXZ.x + waves.horizontalDisplacement.x,
		u_SeaWorldHeight + waves.verticalDisplacement,
		worldXZ.y + waves.horizontalDisplacement.y);
	if (terrainCoverage > 0.5 && sampledWaterDepth > 0.0)
	{
		float minimumSurfaceHeight = terrainHeight + max(sampledWaterDepth * 0.70, 0.0005);
		position.y = max(position.y, minimumSurfaceHeight);
	}

	vertexOutput.worldPosition = position;
	vertexOutput.waveNormal = normalize(vec3(-waves.heightGradient.x, 1.0, -waves.heightGradient.y));
	vertexOutput.texCoord = terrainTexCoord;
	vertexOutput.waterDepth = waterDepth;
	vertexOutput.waveSteepness = clamp(length(waves.heightGradient) * 2.0, 0.0, 1.0);
	vertexOutput.terrainCoverage = terrainCoverage;
	gl_Position = u_ProjectionView * vec4(position, 1.0);
}
