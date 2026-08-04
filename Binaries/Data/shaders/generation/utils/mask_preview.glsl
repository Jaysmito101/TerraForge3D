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
// x = sea level, y = select valleys, z = use path
uniform vec4 u_Settings2;

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
	float softness = max(u_Range.z, 0.000001);
	float lower = smoothstep(u_Range.x - softness, u_Range.x + softness, value);
	float upper = 1.0 - smoothstep(u_Range.y - softness, u_Range.y + softness, value);
	return clamp(lower * upper, 0.0, 1.0);
}

vec2 TerrainGradient(ivec2 coordinate)
{
	float top = TerrainValue(coordinate + ivec2(0, -1));
	float bottom = TerrainValue(coordinate + ivec2(0, 1));
	float left = TerrainValue(coordinate + ivec2(-1, 0));
	float right = TerrainValue(coordinate + ivec2(1, 0));
	float texelSize = max(u_Range.w / float(u_Resolution), 0.000001);
	return vec2(right - left, bottom - top) / (2.0 * texelSize);
}

float SlopeDegrees(ivec2 coordinate)
{
	return degrees(atan(length(TerrainGradient(coordinate))));
}

float CurvatureValue(ivec2 coordinate)
{
	float center = TerrainValue(coordinate);
	float neighbors = TerrainValue(coordinate + ivec2(-1, 0))
		+ TerrainValue(coordinate + ivec2(1, 0))
		+ TerrainValue(coordinate + ivec2(0, -1))
		+ TerrainValue(coordinate + ivec2(0, 1));
	return 4.0 * center - neighbors;
}

float RoughnessValue(ivec2 coordinate)
{
	float center = TerrainValue(coordinate);
	float total = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0) continue;
			total += abs(TerrainValue(coordinate + ivec2(x, y)) - center);
		}
	}
	return total / 8.0;
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

float LocalWetness(ivec2 coordinate)
{
	float height = TerrainValue(coordinate);
	float average = 0.25 * (
		TerrainValue(coordinate + ivec2(-1, 0)) + TerrainValue(coordinate + ivec2(1, 0)) +
		TerrainValue(coordinate + ivec2(0, -1)) + TerrainValue(coordinate + ivec2(0, 1)));
	float lowland = 1.0 - clamp(height * 0.5 + 0.5, 0.0, 1.0);
	float concavity = clamp((average - height) * 8.0 + 0.5, 0.0, 1.0);
	float gentle = 1.0 - clamp(SlopeDegrees(coordinate) / 90.0, 0.0, 1.0);
	return clamp(lowland * 0.45 + concavity * 0.35 + gentle * 0.20, 0.0, 1.0);
}

float AmbientCavity(ivec2 coordinate)
{
	float height = TerrainValue(coordinate);
	float average = 0.125 * (
		TerrainValue(coordinate + ivec2(-1, -1)) + TerrainValue(coordinate + ivec2(0, -1)) +
		TerrainValue(coordinate + ivec2(1, -1)) + TerrainValue(coordinate + ivec2(-1, 0)) +
		TerrainValue(coordinate + ivec2(1, 0)) + TerrainValue(coordinate + ivec2(-1, 1)) +
		TerrainValue(coordinate + ivec2(0, 1)) + TerrainValue(coordinate + ivec2(1, 1)));
	return clamp((average - height) * 8.0 + 0.5, 0.0, 1.0);
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

	switch (u_Mode)
	{
	case 0: return RangeMask(height);
	case 1: return RangeMask(slope); 
	case 2: return DirectionMask(aspect);
	case 3: return RangeMask(curvature); 
	case 4: return RangeMask(RoughnessValue(coordinate)); 
	case 5: return RangeMask(1.0 - clamp(slope / 90.0, 0.0, 1.0)); 
	case 6: return RangeMask(u_Settings2.y > 0.5 ? -curvature : curvature);
	case 7: return RangeMask(height - u_Settings2.x); 
	case 8: return RangeMask(abs(height - u_Settings2.x));
	case 9: return RangeMask(LocalWetness(coordinate)); 
	case 10: return RangeMask(AmbientCavity(coordinate));
	case 11: return RangeMask(0.5 + 0.5 * dot(SafeNormalize(gradient), vec2(cos(radians(u_Settings0.x)), sin(radians(u_Settings0.x))))); 
	case 12:
	{
		float borderDistance = min(min(float(coordinate.x), float(coordinate.y)),
			min(float(u_Resolution - 1 - coordinate.x), float(u_Resolution - 1 - coordinate.y)));
		return RangeMask(borderDistance / max(float(u_Resolution) * 0.5, 1.0));
	}
	case 13:
	{
		float distanceValue = u_Settings2.z > 0.5
			? PointSegmentDistance(uv, u_Settings1.xy, u_Settings1.zw)
			: distance(uv, u_Settings1.xy);
		return RangeMask(distanceValue);
	}
	case 14: return RangeMask(height); 
	case 15: return RangeMask(FractalNoise(uv * max(u_Settings0.z, 0.01) + vec2(u_Settings0.w)));
	case 16: return RangeMask(distance(uv, u_Settings1.xy)); 
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
