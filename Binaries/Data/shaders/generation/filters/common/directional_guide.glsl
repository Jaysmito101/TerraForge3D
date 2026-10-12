#include "terrain_filter_common.glsl"

vec2 tf3dAngleDirection(float angleDegrees)
{
	float angle = radians(angleDegrees);
	return vec2(cos(angle), sin(angle));
}

vec2 tf3dUpwardSlopeAt(ivec2 coordinate)
{
	int slopeStep = max(u_Resolution / 128, 1);
	ivec2 offsetX = ivec2(slopeStep, 0);
	ivec2 offsetY = ivec2(0, slopeStep);
	float inverseSpan = 0.5f / float(slopeStep);
	return vec2(
		sampleInput(coordinate + offsetX) - sampleInput(coordinate - offsetX),
		sampleInput(coordinate + offsetY) - sampleInput(coordinate - offsetY)) * inverseSpan;
}

float tf3dUpwardSlopeAlignment(vec2 upwardSlope, vec2 angleDirection)
{
	vec2 slope = upwardSlope;
	float slopeLengthSquared = dot(slope, slope);
	if (slopeLengthSquared <= 0.000000000001f)
	{
		return 0.0f;
	}

	vec2 upwardHorizontalDirection = slope * inversesqrt(slopeLengthSquared);
	float alignment = clamp(dot(upwardHorizontalDirection, angleDirection), 0.0f, 1.0f);
	float alignmentWeight = alignment * alignment * (3.0f - 2.0f * alignment);
	float slopeMagnitude = length(slope) * float(u_Resolution) / tf3dFieldFullRange();
	float slopeConfidence = smoothstep(0.015f, 0.10f, slopeMagnitude);
	return alignmentWeight * slopeConfidence;
}
