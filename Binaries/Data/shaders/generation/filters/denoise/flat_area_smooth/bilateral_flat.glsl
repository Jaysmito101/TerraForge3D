#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../../common/field_common.glsl"

uniform int u_Radius;
uniform float u_RangeSigma;
uniform float u_SlopeLimit;
uniform float u_SlopeSoftness;

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	int radius = clamp(u_Radius, 1, 6);
	float spatialSigma = max(float(radius) * 0.5f, 0.5f);
	float statisticsRange = tf3dFieldScaleRange();
	float rangeSigma = max(u_RangeSigma * statisticsRange, 0.000001f);
	float center = sampleInput(coordinate);
	float filtered = 0.0f;
	float weightSum = 0.0f;
	for (int y = -6; y <= 6; y++)
	{
		for (int x = -6; x <= 6; x++)
		{
			if (abs(x) > radius || abs(y) > radius) continue;
			float neighbor = sampleInput(coordinate + ivec2(x, y));
			float spatialDistance = float(x * x + y * y);
			float rangeDistance = neighbor - center;
			float spatialWeight = exp(-0.5f * spatialDistance / (spatialSigma * spatialSigma));
			float rangeWeight = exp(-0.5f * rangeDistance * rangeDistance / (rangeSigma * rangeSigma));
			float weight = spatialWeight * rangeWeight;
			filtered += neighbor * weight;
			weightSum += weight;
		}
	}
	filtered /= max(weightSum, 0.000001f);

	float dx = 0.5f * (sampleInput(coordinate + ivec2(1, 0)) - sampleInput(coordinate + ivec2(-1, 0)));
	float dy = 0.5f * (sampleInput(coordinate + ivec2(0, 1)) - sampleInput(coordinate + ivec2(0, -1)));
	float slope = length(vec2(dx, dy)) / statisticsRange;
	float slopeEnd = u_SlopeLimit + max(u_SlopeSoftness, 0.000001f);
	float flatInfluence = 1.0f - smoothstep(max(u_SlopeLimit, 0.0f), slopeEnd, slope);
	writeOutput(coordinate, mix(center, filtered, clamp(flatInfluence, 0.0f, 1.0f)));
}
