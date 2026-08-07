#include "field_common.glsl"

float tf3dSampleFieldBilinear(vec2 coordinate)
{
	vec2 clampedCoordinate = clamp(coordinate, vec2(0.0f), vec2(float(max(u_Resolution - 1, 0))));
	ivec2 minimumCoordinate = ivec2(floor(clampedCoordinate));
	ivec2 maximumCoordinate = min(minimumCoordinate + ivec2(1), ivec2(max(u_Resolution - 1, 0)));
	vec2 fraction = fract(clampedCoordinate);
	float lower = mix(sampleInput(minimumCoordinate), sampleInput(ivec2(maximumCoordinate.x, minimumCoordinate.y)), fraction.x);
	float upper = mix(sampleInput(ivec2(minimumCoordinate.x, maximumCoordinate.y)), sampleInput(maximumCoordinate), fraction.x);
	return mix(lower, upper, fraction.y);
}

vec2 tf3dFieldGradient(ivec2 coordinate)
{
	float horizontal = 0.5f * (sampleInput(coordinate + ivec2(1, 0)) - sampleInput(coordinate + ivec2(-1, 0)));
	float vertical = 0.5f * (sampleInput(coordinate + ivec2(0, 1)) - sampleInput(coordinate + ivec2(0, -1)));
	return vec2(horizontal, vertical);
}

float tf3dFieldSlope(ivec2 coordinate)
{
	return length(tf3dFieldGradient(coordinate)) / tf3dFieldScaleRange();
}

vec2 tf3dFieldDirection(vec2 direction)
{
	float lengthSquared = dot(direction, direction);
	return lengthSquared > 0.0000001f ? direction * inversesqrt(lengthSquared) : vec2(1.0f, 0.0f);
}
