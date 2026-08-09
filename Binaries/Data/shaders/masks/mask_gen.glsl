#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D TerrainData;

layout(binding = 1, r16) uniform image2D u_MaskTexture;

uniform int u_Resolution;
uniform int u_Mode;
uniform float u_TileSize;

/* TF3D_CALCULATED_MASK_UNIFORMS */

// common/noise_2d.glsl uses the canonical Noise* inspector uniform names.
#include "common/noise_2d.glsl"


int PixelCoordToDataOffset(ivec2 coordinate)
{
	return coordinate.y * u_Resolution + coordinate.x;
}

float TerrainValue(ivec2 coordinate)
{
	coordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return imageLoad(TerrainData, coordinate).r;
}

float RangeMask(float value)
{
	float minimum = min(u_Range.x, u_Range.y);
	float maximum = max(u_Range.x, u_Range.y);
	float rangeWidth = max(maximum - minimum, 0.000001);
	float feather = clamp(u_EdgeFeather, 0.0, 0.5);
	if (feather <= 0.000001) return value >= minimum && value <= maximum ? 1.0 : 0.0;
	float normalizedValue = (value - minimum) / rangeWidth;
	float lower = smoothstep(-feather, feather, normalizedValue);
	float upper = 1.0 - smoothstep(1.0 - feather, 1.0 + feather, normalizedValue);
	return clamp(lower * upper, 0.0, 1.0);
}

float ThresholdMask(float value)
{
	float threshold = clamp(u_Threshold, 0.0, 1.0);
	float feather = clamp(u_EdgeFeather, 0.0, 0.5);
	if (feather <= 0.000001) return value >= threshold ? 1.0 : 0.0;
	return smoothstep(threshold - feather, threshold + feather, value);
}

float FilteredTerrainValue(ivec2 coordinate)
{
	float sum = 0.0;
	float weightSum = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			float weight = float((x == 0 ? 2 : 1) * (y == 0 ? 2 : 1));
			sum += TerrainValue(coordinate + ivec2(x, y)) * weight;
			weightSum += weight;
		}
	}
	return sum / weightSum;
}

vec2 TerrainGradient(ivec2 coordinate)
{
	int sampleRadius = clamp(int(round(u_SampleRadius)), 1, 8);
	float texelSize = max(u_TileSize / float(u_Resolution), 0.000001);
	float step = float(sampleRadius);
	float dX =
		(3.0 * FilteredTerrainValue(coordinate + ivec2( sampleRadius, -sampleRadius)) +
		 10.0 * FilteredTerrainValue(coordinate + ivec2( sampleRadius,  0)) +
		  3.0 * FilteredTerrainValue(coordinate + ivec2( sampleRadius,  sampleRadius)) -
		  3.0 * FilteredTerrainValue(coordinate + ivec2(-sampleRadius, -sampleRadius)) -
		 10.0 * FilteredTerrainValue(coordinate + ivec2(-sampleRadius,  0)) -
		  3.0 * FilteredTerrainValue(coordinate + ivec2(-sampleRadius,  sampleRadius))) /
		(32.0 * step * texelSize);
	float dY =
		(3.0 * FilteredTerrainValue(coordinate + ivec2(-sampleRadius,  sampleRadius)) +
		 10.0 * FilteredTerrainValue(coordinate + ivec2( 0,  sampleRadius)) +
		  3.0 * FilteredTerrainValue(coordinate + ivec2( sampleRadius,  sampleRadius)) -
		  3.0 * FilteredTerrainValue(coordinate + ivec2(-sampleRadius, -sampleRadius)) -
		 10.0 * FilteredTerrainValue(coordinate + ivec2( 0, -sampleRadius)) -
		  3.0 * FilteredTerrainValue(coordinate + ivec2( sampleRadius, -sampleRadius))) /
		(32.0 * step * texelSize);
	return vec2(dX, dY);
}

float SlopeDegrees(ivec2 coordinate)
{
	return degrees(atan(length(TerrainGradient(coordinate))));
}

float SlopeRampMask(float slope)
{
	float normalizedSlope = clamp(slope / 90.0, 0.0, 1.0);
	float minimum = min(u_Range.x, u_Range.y);
	float maximum = max(u_Range.x, u_Range.y);
	if (maximum - minimum <= 0.000001)
		return normalizedSlope >= minimum ? 1.0 : 0.0;

	float ramp = smoothstep(minimum, maximum, normalizedSlope);
	return u_Range.x <= u_Range.y ? ramp : 1.0 - ramp;
}

float CurvatureValue(ivec2 coordinate)
{
	float center = FilteredTerrainValue(coordinate);
	float neighbors = FilteredTerrainValue(coordinate + ivec2(-1, 0))
		+ FilteredTerrainValue(coordinate + ivec2(1, 0))
		+ FilteredTerrainValue(coordinate + ivec2(0, -1))
		+ FilteredTerrainValue(coordinate + ivec2(0, 1));
	return 4.0 * center - neighbors;
}

float CurvatureMagnitude(float curvature)
{
	float response = 1.0 - exp(-abs(curvature) * max(u_CurvatureSensitivity, 0.001));
	return clamp(response, 0.0, 1.0);
}

float RidgeValleyStrength(float curvature)
{
	return 1.0 - exp(-max(curvature, 0.0) * max(u_CurvatureSensitivity, 0.001));
}

float RoughnessValue(ivec2 coordinate)
{
	float center = FilteredTerrainValue(coordinate);
	float total = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0) continue;
			total += abs(FilteredTerrainValue(coordinate + ivec2(x, y)) - center);
		}
	}
	float average = total / 8.0;
	return 1.0 - exp(-average * 16.0);
}

float AngleDistance(float a, float b)
{
	float wrapped = mod(a - b + 180.0, 360.0);
	if (wrapped < 0.0) wrapped += 360.0;
	return abs(wrapped - 180.0);
}

float DirectionMask(float angle)
{
	float width = clamp(u_DirectionWidth, 0.0, 180.0);
	float distance = AngleDistance(angle, u_Direction);
	float softness = max(u_EdgeSoftness, 0.000001);
	softness = min(softness, width > 0.001 ? width * 0.5 : 5.0);
	return 1.0 - smoothstep(width, width + softness, distance);
}

float PointSegmentDistance(vec2 point, vec2 start, vec2 end)
{
	vec2 direction = end - start;
	float lengthSquared = dot(direction, direction);
	float amount = lengthSquared > 0.000001 ? clamp(dot(point - start, direction) / lengthSquared, 0.0, 1.0) : 0.0;
	return distance(point, start + direction * amount);
}

float PointPathDistance(vec2 point)
{
	float result = 1.0e20;
	int pointCount = clamp(u_PathPointsCount, 2, 16);
	for (int pointIndex = 0; pointIndex < 15; ++pointIndex)
	{
		if (pointIndex + 1 >= pointCount) break;
		result = min(result, PointSegmentDistance(point, u_PathPoints[pointIndex], u_PathPoints[pointIndex + 1]));
	}
	return result;
}

float LocalWetness(ivec2 coordinate)
{
	float height = FilteredTerrainValue(coordinate);
	float average = 0.25 * (
		FilteredTerrainValue(coordinate + ivec2(-1, 0)) + FilteredTerrainValue(coordinate + ivec2(1, 0)) +
		FilteredTerrainValue(coordinate + ivec2(0, -1)) + FilteredTerrainValue(coordinate + ivec2(0, 1)));
	float lowland = 1.0 - clamp(height * 0.5 + 0.5, 0.0, 1.0);
	float concavity = clamp((average - height) * 8.0 + 0.5, 0.0, 1.0);
	float gentle = 1.0 - clamp(SlopeDegrees(coordinate) / 90.0, 0.0, 1.0);
	return clamp(lowland * 0.45 + concavity * 0.35 + gentle * 0.20, 0.0, 1.0);
}

float AmbientCavity(ivec2 coordinate)
{
	float height = FilteredTerrainValue(coordinate);
	const int radius = 2;
	float average = 0.125 * (
		FilteredTerrainValue(coordinate + ivec2(-radius, -radius)) + FilteredTerrainValue(coordinate + ivec2(0, -radius)) +
		FilteredTerrainValue(coordinate + ivec2(radius, -radius)) + FilteredTerrainValue(coordinate + ivec2(-radius, 0)) +
		FilteredTerrainValue(coordinate + ivec2(radius, 0)) + FilteredTerrainValue(coordinate + ivec2(-radius, radius)) +
		FilteredTerrainValue(coordinate + ivec2(0, radius)) + FilteredTerrainValue(coordinate + ivec2(radius, radius)));
	float depression = max(average - height, 0.0);
	return 1.0 - exp(-depression * max(u_CavitySensitivity, 0.001));
}

float SpiralMask(vec2 uv)
{
	const float twoPi = 6.28318530718;
	vec2 offset = uv - u_Center;
	float radius = length(offset);
	float angle = atan(offset.y, offset.x);
	float arms = max(u_SpiralArms, 1.0);
	float turns = max(u_SpiralTurns, 0.0);
	float phase = angle / twoPi * arms + radius * turns - u_SpiralRotation / 360.0;
	float distanceToArm = abs(fract(phase + 0.5) - 0.5) * 2.0;
	float halfWidth = clamp(u_SpiralThickness, 0.001, 0.5);
	float antiAlias = max((arms + turns) / max(float(u_Resolution), 1.0), 0.0001);
	float softness = max(u_SpiralSoftness, antiAlias);
	float mask = 1.0 - smoothstep(halfWidth, halfWidth + softness, distanceToArm);
	return u_SpiralInvert != 0 ? 1.0 - mask : mask;
}

float GridMask(vec2 uv)
{
	const float twoPi = 6.28318530718;
	float rotation = u_GridRotation / 360.0 * twoPi;
	mat2 rotationMatrix = mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
	vec2 gridPosition = rotationMatrix * (uv - u_Center) * max(u_GridCells, 1.0);
	vec2 distanceToLine = abs(fract(gridPosition + 0.5) - 0.5);
	float lineDistance = min(distanceToLine.x, distanceToLine.y);
	float halfWidth = clamp(u_GridThickness, 0.001, 0.5);
	float antiAlias = max(max(u_GridCells, 1.0) / max(float(u_Resolution), 1.0), 0.0001);
	float softness = max(u_GridSoftness, antiAlias);
	float mask = 1.0 - smoothstep(halfWidth, halfWidth + softness, lineDistance);
	return u_GridInvert != 0 ? 1.0 - mask : mask;
}

float DotMask(vec2 uv)
{
	const float twoPi = 6.28318530718;
	float rotation = u_DotRotation / 360.0 * twoPi;
	mat2 rotationMatrix = mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
	vec2 dotPosition = rotationMatrix * (uv - u_Center) * max(u_DotCells, 1.0);
	vec2 cellOffset = fract(dotPosition + 0.5) - 0.5;
	float distanceToDot = length(cellOffset);
	float radius = clamp(u_DotRadius, 0.001, 0.5);
	float antiAlias = max(max(u_DotCells, 1.0) / max(float(u_Resolution), 1.0), 0.0001);
	float softness = max(u_DotSoftness, antiAlias);
	float mask = 1.0 - smoothstep(radius, radius + softness, distanceToDot);
	return u_DotInvert != 0 ? 1.0 - mask : mask;
}

vec2 SafeNormalize(vec2 value)
{
	float lengthValue = length(value);
	return lengthValue > 0.000001 ? value / lengthValue : vec2(0.0);
}

struct MaskContext
{
	ivec2 coordinate;
	vec2 uv;
};

/* TF3D_CALCULATED_MASK_MODULES */

float TerrainMask(ivec2 coordinate, vec2 uv)
{
	MaskContext context;
	context.coordinate = coordinate;
	context.uv = uv;
	return tf3d_calculated_mask_dispatch(context);
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;
	vec2 uv = (vec2(coordinate) + vec2(0.5)) / float(u_Resolution);
	imageStore(u_MaskTexture, coordinate, vec4(TerrainMask(coordinate, uv), 0.0, 0.0, 1.0));
}
