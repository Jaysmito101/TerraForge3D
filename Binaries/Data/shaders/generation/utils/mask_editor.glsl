#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, r16) uniform image2D u_MaskTexture;
layout(binding = 0) uniform sampler2D u_BaseMask;

layout(std430, binding = 1) readonly buffer StrokeSettingsBuffer
{
	vec4 u_StrokeSettings[]; // strength, size, falloff, mode
};

layout(std430, binding = 2) readonly buffer StrokeRangesBuffer
{
	ivec4 u_StrokeRanges[]; // point start, point count, unused, unused
};

layout(std430, binding = 3) readonly buffer StrokePointsBuffer
{
	vec4 u_StrokePoints[]; // normalized terrain position in xy
};

uniform int u_Resolution;
uniform int u_StrokeCount;

float distanceToStroke(in vec2 uv, in int pointStart, in int pointCount)
{
	if (pointCount <= 0) return 1000000.0f;
	if (pointCount == 1) return length(uv - u_StrokePoints[pointStart].xy);

	float closestDistance = 1000000.0f;
	for (int pointIndex = 0; pointIndex < pointCount - 1; ++pointIndex)
	{
		vec2 start = u_StrokePoints[pointStart + pointIndex].xy;
		vec2 end = u_StrokePoints[pointStart + pointIndex + 1].xy;
		vec2 segment = end - start;
		float segmentLengthSquared = dot(segment, segment);
		float projection = segmentLengthSquared < 0.0000001f
			? 0.0f
			: clamp(dot(uv - start, segment) / segmentLengthSquared, 0.0f, 1.0f);
		closestDistance = min(closestDistance, length(uv - (start + projection * segment)));
	}
	return closestDistance;
}

float strokeInfluence(in vec2 uv, in vec4 settings, in int pointStart, in int pointCount)
{
	float distanceValue = distanceToStroke(uv, pointStart, pointCount);
	float outerEdge = max(settings.y, 0.000001f);
	float innerEdge = outerEdge * (1.0f - clamp(settings.z, 0.0f, 1.0f));
	float falloff = smoothstep(innerEdge, max(innerEdge + 0.000001f, outerEdge), distanceValue);
	return clamp(settings.x * (1.0f - falloff), 0.0f, 1.0f);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;

	float value = texelFetch(u_BaseMask, coordinate, 0).r;
	vec2 uv = (vec2(coordinate) + vec2(0.5f)) / float(u_Resolution);

	// NOTE/TODO: This is a pretty bad ideas as this scales very badly, ideally we want
	// either a more clever way to select strokes per pixel (like some partationng)
	// or we rasterize strokes incrementally
	for (int strokeIndex = 0; strokeIndex < u_StrokeCount; ++strokeIndex)
	{
		vec4 settings = u_StrokeSettings[strokeIndex];
		ivec4 range = u_StrokeRanges[strokeIndex];
		float influence = strokeInfluence(uv, settings, range.x, range.y);
		float target = settings.w > 0.5f ? 0.0f : 1.0f;
		value = mix(value, target, influence);
	}

	imageStore(u_MaskTexture, coordinate, vec4(value, value, value, 1.0f));
}
