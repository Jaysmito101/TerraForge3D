#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/terrain_filter_common.glsl"

uniform int u_Mode;
uniform int u_Radius;
uniform float u_SlopeLimit;
uniform float u_SlopeSoftness;

const float kReferenceResolution = 1024.0f;

float tf3dBroadSlope(ivec2 coordinate, float radius, float fieldScaleRange)
{
	float referenceScale = float(u_Resolution) / kReferenceResolution;
	float sampleRadius = max(radius * 0.25f, referenceScale);
	vec2 offsetX = vec2(sampleRadius, 0.0f);
	vec2 offsetY = vec2(0.0f, sampleRadius);
	vec2 gradient = vec2(
		tf3dSampleFieldBilinear(vec2(coordinate) + offsetX) - tf3dSampleFieldBilinear(vec2(coordinate) - offsetX),
		tf3dSampleFieldBilinear(vec2(coordinate) + offsetY) - tf3dSampleFieldBilinear(vec2(coordinate) - offsetY));
	return length(gradient) * referenceScale / (2.0f * sampleRadius * fieldScaleRange);
}

float tf3dSoftMorphologicalTarget(ivec2 coordinate, float center, float radius, int mode, float fieldScaleRange)
{
	const vec2 directions[16] = vec2[16](
		vec2(1.0f, 0.0f),
		vec2(0.9238795f, 0.3826834f),
		vec2(0.7071068f, 0.7071068f),
		vec2(0.3826834f, 0.9238795f),
		vec2(0.0f, 1.0f),
		vec2(-0.3826834f, 0.9238795f),
		vec2(-0.7071068f, 0.7071068f),
		vec2(-0.9238795f, 0.3826834f),
		vec2(-1.0f, 0.0f),
		vec2(-0.9238795f, -0.3826834f),
		vec2(-0.7071068f, -0.7071068f),
		vec2(-0.3826834f, -0.9238795f),
		vec2(0.0f, -1.0f),
		vec2(0.3826834f, -0.9238795f),
		vec2(0.7071068f, -0.7071068f),
		vec2(0.9238795f, -0.3826834f));
	const float radialFractions[2] = float[2](0.5f, 1.0f);

	float sign = mode == 0 ? 1.0f : -1.0f;
	float softness = max(fieldScaleRange * 0.02f, 0.000001f);
	float distancePenalty = fieldScaleRange * 0.001f;
	float centerScore = sign * center;
	float weightedScore = centerScore;
	float weightSum = 1.0f;

	for (int directionIndex = 0; directionIndex < 16; ++directionIndex)
	{
		for (int radialIndex = 0; radialIndex < 2; ++radialIndex)
		{
			float radialFraction = radialFractions[radialIndex];
			vec2 sampleCoordinate = vec2(coordinate) + directions[directionIndex] * (float(radius) * radialFraction);
			float sampleValue = tf3dSampleFieldBilinear(sampleCoordinate);
			float candidateScore = sign * sampleValue - distancePenalty * radialFraction * radialFraction;
			float sampleWeight = clamp(0.5f + 0.5f * (candidateScore - centerScore) / softness, 0.0f, 1.0f);
			weightedScore += candidateScore * sampleWeight;
			weightSum += sampleWeight;
		}
	}

	float target = sign * (weightedScore / weightSum);
	return mode == 0 ? max(center, target) : min(center, target);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution)
	{
		return;
	}

	float center = sampleInput(coordinate);
	float resolutionScale = float(u_Resolution) / kReferenceResolution;
	float radius = float(clamp(u_Radius, 1, 64)) * resolutionScale;
	float fieldScaleRange = tf3dFieldScaleRange();
	float slope = tf3dBroadSlope(coordinate, radius, fieldScaleRange);
	float slopeSoftness = max(u_SlopeSoftness, 0.000001f);
	float slopeBias = smoothstep(max(u_SlopeLimit, 0.0f), max(u_SlopeLimit, 0.0f) + slopeSoftness, slope);
	float pushGain = mix(1.0f, 2.0f, slopeBias);
	float target = tf3dSoftMorphologicalTarget(coordinate, center, radius, u_Mode, fieldScaleRange);
	writeOutput(coordinate, center + (target - center) * pushGain);
}
