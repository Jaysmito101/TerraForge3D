#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer TerrainData
{
	float data0[];
};

layout(binding = 1, r16) uniform image2D u_MaskTexture;

uniform int u_Resolution;
uniform int u_Mode;
// x = minimum, y = maximum, z = edge softness, w = full tile size
uniform vec4 u_Range;
// x = direction angle, y = direction width, z = procedural scale, w = seed
uniform vec4 u_Settings0;
// xy = point/center, zw = path endpoint
uniform vec4 u_Settings1;
// x = sea level, y = select valleys, z = use path, w = terrain sample radius
uniform vec4 u_Settings2;
uniform float u_CurvatureScale;
uniform float u_CavityScale;
uniform vec2 u_PathPoints[16];
uniform int u_PathPointCount;


int PixelCoordToDataOffset(ivec2 coordinate)
{
	return coordinate.y * u_Resolution + coordinate.x;
}

float TerrainValue(ivec2 coordinate)
{
	coordinate = clamp(coordinate, ivec2(0), ivec2(u_Resolution - 1));
	return data0[PixelCoordToDataOffset(coordinate)];
}

float RangeMask(float value)
{
	float minimum = min(u_Range.x, u_Range.y);
	float maximum = max(u_Range.x, u_Range.y);
	float rangeWidth = max(maximum - minimum, 0.000001);
	float feather = clamp(u_Range.z, 0.0, 0.5);
	if (feather <= 0.000001) return value >= minimum && value <= maximum ? 1.0 : 0.0;
	float normalizedValue = (value - minimum) / rangeWidth;
	float lower = smoothstep(-feather, feather, normalizedValue);
	float upper = 1.0 - smoothstep(1.0 - feather, 1.0 + feather, normalizedValue);
	return clamp(lower * upper, 0.0, 1.0);
}

float ThresholdMask(float value)
{
	float threshold = clamp(u_Range.x, 0.0, 1.0);
	float feather = clamp(u_Range.z, 0.0, 0.5);
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
	int sampleRadius = clamp(int(round(u_Settings2.w)), 1, 8);
	float texelSize = max(u_Range.w / float(u_Resolution), 0.000001);
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
	float response = 1.0 - exp(-abs(curvature) * max(u_CurvatureScale, 0.001));
	return clamp(response, 0.0, 1.0);
}

float RidgeValleyStrength(float curvature)
{
	return 1.0 - exp(-max(curvature, 0.0) * max(u_CurvatureScale, 0.001));
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
	float width = clamp(u_Settings0.y, 0.0, 180.0);
	float distance = AngleDistance(angle, u_Settings0.x);
	float softness = max(u_Range.z, 0.000001);
	softness = min(softness, width > 0.001 ? width * 0.5 : 5.0);
	return 1.0 - smoothstep(width, width + softness, distance);
}

float Hash12(vec2 position)
{
	vec3 value = fract(vec3(position.xyx) * 0.1031);
	value += dot(value, value.yzx + 33.33);
	return fract((value.x + value.y) * value.z);
}

float Noise2(vec2 position)
{
	vec2 cell = floor(position);
	vec2 local = fract(position);
	local = local * local * (3.0 - 2.0 * local);
	float a = Hash12(cell);
	float b = Hash12(cell + vec2(1.0, 0.0));
	float c = Hash12(cell + vec2(0.0, 1.0));
	float d = Hash12(cell + vec2(1.0, 1.0));
	return mix(mix(a, b, local.x), mix(c, d, local.x), local.y);
}

float FractalNoise(vec2 position)
{
	float value = 0.0;
	float amplitude = 0.5;
	float amplitudeSum = 0.0;
	for (int octave = 0; octave < 5; ++octave)
	{
		value += Noise2(position) * amplitude;
		amplitudeSum += amplitude;
		position = position * 2.03 + vec2(17.13, 9.71);
		amplitude *= 0.5;
	}
	return value / max(amplitudeSum, 0.000001);
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
	int pointCount = clamp(u_PathPointCount, 2, 16);
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
	return 1.0 - exp(-depression * max(u_CavityScale, 0.001));
}

vec2 SafeNormalize(vec2 value)
{
	float lengthValue = length(value);
	return lengthValue > 0.000001 ? value / lengthValue : vec2(0.0);
}

float TerrainMask(ivec2 coordinate, vec2 uv)
{
	float height = TerrainValue(coordinate);
	float slope = SlopeDegrees(coordinate);
	vec2 gradient = TerrainGradient(coordinate);
	float aspect = mod(degrees(atan(gradient.y, gradient.x)) + 360.0, 360.0);
	float curvature = CurvatureValue(coordinate);
	float curvatureMagnitude = CurvatureMagnitude(curvature);

	switch (u_Mode)
	{
	case TF3D_MASK_HEIGHT_RANGE: return RangeMask(height);
	case TF3D_MASK_SLOPE_RANGE: return RangeMask(slope);
	case TF3D_MASK_ASPECT: return DirectionMask(aspect);
	case TF3D_MASK_CURVATURE: return RangeMask(curvatureMagnitude);
	case TF3D_MASK_ROUGHNESS: return RangeMask(RoughnessValue(coordinate));
	case TF3D_MASK_FLATNESS: return RangeMask(1.0 - clamp(slope / 90.0, 0.0, 1.0));
	case TF3D_MASK_RIDGE_VALLEY: return ThresholdMask(RidgeValleyStrength(u_Settings2.y > 0.5 ? -curvature : curvature));
	case TF3D_MASK_COASTLINE: return RangeMask(height - u_Settings2.x);
	case TF3D_MASK_DISTANCE_FROM_COAST: return RangeMask(abs(height - u_Settings2.x));
	case TF3D_MASK_FLOW_WETNESS: return RangeMask(LocalWetness(coordinate));
	case TF3D_MASK_AMBIENT_OCCLUSION: return RangeMask(AmbientCavity(coordinate));
	case TF3D_MASK_EXPOSURE: return RangeMask(0.5 + 0.5 * dot(SafeNormalize(gradient), vec2(cos(radians(u_Settings0.x)), sin(radians(u_Settings0.x)))));
	case TF3D_MASK_DISTANCE_FROM_BORDER:
	{
		float borderDistance = min(min(float(coordinate.x), float(coordinate.y)),
			min(float(u_Resolution - 1 - coordinate.x), float(u_Resolution - 1 - coordinate.y)));
		return RangeMask(borderDistance / max(float(u_Resolution) * 0.5, 1.0));
	}
	case TF3D_MASK_DISTANCE_FROM_POINT_PATH:
	{
		float distanceValue = u_Settings2.z > 0.5
			? PointPathDistance(uv)
			: distance(uv, u_Settings1.xy);
		return RangeMask(distanceValue);
	}
	case TF3D_MASK_HEIGHT_CONTOUR: return RangeMask(height);
	case TF3D_MASK_PROCEDURAL_NOISE: return RangeMask(FractalNoise(uv * max(u_Settings0.z, 0.01) + vec2(u_Settings0.w)));
	case TF3D_MASK_RADIAL_GRADIENT:
	{
		float radialDistance = distance(uv, u_Settings1.xy);
		float innerRadius = min(u_Range.x, u_Range.y);
		float outerRadius = max(u_Range.x, u_Range.y);
		float radialFeather = clamp(u_Range.z, 0.0, 0.5);
		float radialSoftness = (outerRadius - innerRadius) * radialFeather;
		if (outerRadius - innerRadius <= 0.000001) return radialDistance <= outerRadius ? 1.0 : 0.0;
		return 1.0 - smoothstep(innerRadius - radialSoftness, outerRadius + radialSoftness, radialDistance);
	}
	default: return 0.0;
	}
}

void main()
{
	ivec2 coordinate = ivec2(gl_GlobalInvocationID.xy);
	if (coordinate.x >= u_Resolution || coordinate.y >= u_Resolution) return;
	vec2 uv = (vec2(coordinate) + vec2(0.5)) / float(u_Resolution);
	imageStore(u_MaskTexture, coordinate, vec4(TerrainMask(coordinate, uv), 0.0, 0.0, 1.0));
}
